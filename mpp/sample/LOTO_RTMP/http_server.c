#include "http_server.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#include "ConfigParser.h"
#include "common.h"
#include "Loto_venc.h"
#include "loto_cover.h"

#define DEBUG_HTTP 0

#define MAX_PENDING           10
#define MAX_RESPONSE_SIZE     5120
#define MAX_HTML_CONTENT_SIZE 4096

#define HTML_FILE_PATH   "html/index.html"
#define DEVICE_FILE_PATH "html/device.html"

#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: loto_hisidv300/1.0.0\r\n"

#define RESPONSE_TEMPLATE                      \
    "HTTP/1.1 200 OK\r\n"                      \
    "Content-Type: text/%s; charset=utf-8\r\n" \
    "Content-Length: %d\r\n"                   \
    "Connection: keep-alive\r\n"               \
    "\r\n"                                     \
    "%s"

#define JPG_RESPONSE_TEMPLATE      \
    "HTTP/1.1 200 OK\r\n"          \
    "Content-Type: image/jpeg\r\n" \
    "Content-Length: %d\r\n"       \
    "\r\n"                         \

typedef struct KeyValuePair {
    char* key;
    char* value;
} KeyValuePair;

static int gs_reboot_switch = 0;

extern time_t     g_program_start_time;
extern DeviceInfo g_device_info;

/**
 * @brief 打印出错误信息并结束程序
 *
 * @param sc 错误信息
 */
void error_die(const char *sc)
{
    perror(sc);
    exit(1);
}

/**
 * @brief 读取一行 http 报文，以 '\r' 或 '\r\n' 为行结束符
 *        注：只是把 '\r\n' 之前的内容读取到 buf 中，最后再加一个'\n\0'
 *
 * @param sock socket fd
 * @param buf 缓存指针
 * @param size 读取最大值
 * @return int 读取到的字符个数,不包括'\0'
 */
int get_request_line(int sock, char *buf, int size)
{
    int  i = 0;
    char c = '\0';
    int  n;

    // 读取到的字符个数大于size或者读到\n结束循环
    while ((i < size - 1) && (c != '\n')) {
        n = recv(sock, &c, 1, 0); // 接收一个字符
        // DEBUG printf("%02X\n", c);
        if (n > 0) {
            if (c == '\r') // 回车字符
            {
                // 使用 MSG_PEEK
                // 标志使下一次读取依然可以得到这次读取的内容，可认为接收窗口不滑动
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */

                // 读取到回车换行
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1,
                         0); // 还需要读取，因为之前一次的读取，相当于没有读取
                else
                    c = '\n'; // 如果只读取到\r，也要终止读取
            }
            // 没有读取到\r,则把读取到内容放在buf中
            buf[i] = c;
            i++;
        } else
            c = '\n';
    }

    buf[i] = '\0';

    return i;
}

/**
 * @brief 返回给浏览器表明收到的 HTTP 请求所用的 method 不被支持，501
 *
 * @param client client_socket_fd
 */
void unimplemented(int client)
{
    char buf[1024] = {0};

    sprintf(buf, "HTTP/1.1 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, SERVER_STRING); // Server: jdbhttpd/0.1.0\r\n
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**
 * @brief 告知客户端404错误(没有找到)
 *
 * @param client client_socket_fd
 */
void not_found(int client)
{
    char buf[1024] = {0};

    sprintf(buf, "HTTP/1.1 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**
 * @brief 把文件 resource 中的数据读取到client中
 *
 * @param client client_socket_fd
 * @param resource 需要传输的文件指针
 */
void cat(int client, FILE *resource)
{
    char buf[1024] = {0};

    fgets(buf, sizeof(buf), resource);
    while (!feof(resource)) {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

// 告知客户端该请求有错误400
void bad_request(int client)
{
    char buf[1024] = {0};

    strcat(buf, "HTTP/1.1 400 BAD REQUEST\r\n");
    strcat(buf, "Content-type: text/plain\r\n");
    strcat(buf, "\r\n");
    strcat(buf, "Your browser sent a bad request, ");
    strcat(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

void parse_request_line(char *request_line, int request_line_len, char *method, char *url, char *version)
{
    size_t i = 0, j = 0;

    // 获取 method
    while (!ISspace(request_line[j]) && (i < request_line_len)) {
        method[i] = request_line[j];
        i++;
        j++;
    }
    method[i] = '\0';

    while (ISspace(request_line[j]) && (j < request_line_len))
        j++;

    // 获取 url
    i = 0;
    while (!ISspace(request_line[j]) && (i < request_line_len) && (j < request_line_len)) {
        url[i] = request_line[j];
        i++;
        j++;
    }
    url[i] = '\0';

    while (ISspace(request_line[j]) && (j < request_line_len))
        j++;

    // 获取 version
    i = 0;
    while (!ISspace(request_line[j]) && (i < request_line_len) && (j < request_line_len)) {
        version[i] = request_line[j];
        i++;
        j++;
    }
    version[i] = '\0';

#if DEBUG_HTTP
    LOGD("Method: %s\n", method);
    LOGD("Url: %s\n", url);
    LOGD("Version: %s\n", version);
#endif
}

void parse_path_with_params(const char *url, char *path, char *query_string)
{
    char *delimiter = strchr(url, '?'); // 查找路径中的问号位置

    if (delimiter != NULL) {
        // char* path_only = strndup(url, delimiter - url); // 提取不带参数的路径部分
        // strcpy(path, path_only);
        // free(path_only);
        strncpy(path, url, delimiter - url);
        strcpy(query_string, delimiter + 1);

#if DEBUG_HTTP
        LOGD("Path: %s\n", path);
        LOGD("Query string: %s\n", query_string);
#endif
    } else {
        strcpy(path, url);

#if DEBUG_HTTP
        LOGD("Path: %s\n", url);
#endif
    }
}



// KeyValuePair* parse_query_string(char* query_string, int* pairs_num) {
int parse_query_string(char *query_string, int *pairs_num, KeyValuePair *pairs)
{
    const char *delimiter = "&";
    const char *equals    = "=";
    int         max_pairs = 0; // Maximum number of key-value pairs, adjust as needed

    // Calculate the number of substrings after splitting
    int size = strlen(query_string);
    int i = 0;

    for (i = 0; i < size; i++) {
        //
        if ((query_string[i] != *delimiter) && (query_string[i + 1] == *delimiter || query_string[i + 1] == '\0'))
            max_pairs++;
    }

#if DEBUG_HTTP
    LOGD("max_pairs: %d\n", max_pairs);
#endif

    // KeyValuePair* pairs = malloc(sizeof(KeyValuePair) * (max_pairs));
    // if (pairs == NULL) {
    //     LOGE("Failed to allocate memory for KeyValuePair\n");
    //     return NULL;
    // }

    int   pair_count  = 0;
    char *query_token = query_string;
    // char*         query_string_copy = strdup(query_string);
    // char*         query_token       = query_string_copy;

    while (query_token != NULL && pair_count < max_pairs) {
        char *pair_end = strchr(query_token, *delimiter);

        if (pair_end != NULL) {
            *pair_end = '\0';
        }

        char *key_end = strchr(query_token, *equals);

        if (key_end != NULL) {
            *key_end          = '\0';
            char *value_start = key_end + 1;
            char *value_end   = strchr(value_start, '\0');

            if (value_start != value_end) {
                // pairs[pair_count].key = strdup(query_token);
                // pairs[pair_count].value = strdup(value_start);
                pairs[pair_count].key   = query_token;
                pairs[pair_count].value = value_start;
                pair_count++;
            }
        }

        query_token = (pair_end != NULL) ? (pair_end + 1) : NULL;
    }

    // free(query_string_copy);

    *pairs_num = pair_count;
    // return pairs;
    return 0;
}

void free_query_string_pairs(KeyValuePair *pairs, int pairs_num)
{
    if (pairs != NULL) {
        int i = 0;
        for (i = 0; i < pairs_num; i++) {
            if (pairs[i].key != NULL) {
                free(pairs[i].key);
                pairs[i].key = NULL;
            }
            if (pairs[i].value != NULL) {
                free(pairs[i].value);
                pairs[i].value = NULL;
            }
        }
        free(pairs);
    }
}

int deal_query_string(char *query_string, char *content)
{
    int ret       = 0;
    int pairs_num = 0;

    // KeyValuePair* pairs = NULL;
    // pairs = parse_query_string(query_string, &pairs_num);

    KeyValuePair pairs[32];
    memset(pairs, 32, sizeof(KeyValuePair));
    parse_query_string(query_string, &pairs_num, pairs);

#if DEBUG_HTTP
    LOGD("Number of pairs: %d\n", pairs_num);
#endif

    char temp[1024] = {0};
    int i = 0;

    for (i = 0; i < pairs_num; i++) {
        if (pairs[i].key != NULL && pairs[i].value != NULL) {

#if DEBUG_HTTP
            LOGD("Key: %s, Value: %s\n", pairs[i].key, pairs[i].value);
#endif

            if (strcasecmp(pairs[i].key, "cover") == 0) {
                if (strcmp(pairs[i].value, "1") == 0) {
                    LOTO_COVER_Switch(COVER_ON);
                    // PutConfigKeyValue("push", "video_state", "off", PUSH_CONFIG_FILE_PATH);

                    sprintf(temp, "Add cover\n");
                    strcat(content, temp);
                    temp[0] = '\0';

                } else if (strcmp(pairs[i].value, "0") == 0) {
                    LOTO_COVER_Switch(COVER_OFF);
                    // PutConfigKeyValue("push", "video_state", "on", PUSH_CONFIG_FILE_PATH);

                    sprintf(temp, "Remove cover\n");
                    strcat(content, temp);
                    temp[0] = '\0';

                } else {
                    LOGD("value[%s] of key[%s] is wrong\n", pairs[i].value, pairs[i].key);
                    ret--;
                }

            } else if (strcasecmp(pairs[i].key, "server_url") == 0) {
                int server_url_status = -1;

                if (strcmp(g_device_info.server_url, TEST_SERVER_URL) == 0) {
                    server_url_status = 0;

                } else if (strcmp(g_device_info.server_url, OFFI_SERVER_URL) == 0) {
                    server_url_status = 1;

                } else {
                    LOGE("server_url read from push.conf is wrong!\n");
                }

                if (strcmp(pairs[i].value, "0") == 0) {
                    if (server_url_status == 1) {
                        PutConfigKeyValue("push", "server_url", TEST_SERVER_URL, PUSH_CONFIG_FILE_PATH);

                        LOGI("Set server_url to TEST_SERVER_URL\n");
                        sprintf(temp, "Set server_url to TEST_SERVER_URL\n");
                        strcat(content, temp);
                        temp[0] = '\0';

                        gs_reboot_switch = 1;

                    } else {
                        LOGI("remain the setting of server_url\n");
                        sprintf(temp, "remain the setting of server_url\n");
                        strcat(content, temp);
                        temp[0] = '\0';
                    }

                } else if (strcmp(pairs[i].value, "1") == 0) {
                    if (server_url_status == 0) {
                        PutConfigKeyValue("push", "server_url", OFFI_SERVER_URL, PUSH_CONFIG_FILE_PATH);

                        LOGI("Set server_url to OFFI_SERVER_URL\n");
                        sprintf(temp, "Set server_url to OFFI_SERVER_URL\n");
                        strcat(content, temp);
                        temp[0] = '\0';

                        gs_reboot_switch = 1;

                    } else {
                        LOGI("remain the setting of server_url\n");
                        sprintf(temp, "remain the setting of server_url\n");
                        strcat(content, temp);
                        temp[0] = '\0';
                    }
                } else {
                    LOGE("value[%s] of key[%s] is wrong\n", pairs[i].value, pairs[i].key);
                    ret--;
                }
            } else {
                LOGE("key[%s] is wrong\n", pairs[i].key);
                ret--;
            }
        }
    }

    return ret;
}

/**
 * @brief server向client 发送响应头部信息
 *
 * @param client client_socket_fd
 * @param content_type 内容类型：html or plain
 */
void send_header(int client, const char *content_type, int content_length)
{
    char header[1024];
    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "\r\n",
            content_type, content_length);

    send(client, header, strlen(header), MSG_NOSIGNAL);
}

// 函数用于读取HTML文件的内容
char *read_html_file(const char *file_path, int *content_length)
{
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        return NULL;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // 分配足够的内存来存储文件内容
    char *content = (char *)malloc(file_size + 1);
    if (content == NULL) {
        perror("malloc");
        fclose(file);
        return NULL;
    }

    // 读取文件内容
    size_t bytes_read = fread(content, 1, file_size, file);
    if (bytes_read != file_size) {
        perror("fread");
        fclose(file);
        free(content);
        return NULL;
    }

    // 添加字符串结束符
    content[file_size] = '\0';

    fclose(file);
    *content_length = file_size + 1;

    return content;
}

void write_html_file(const char *content, const char *file_path)
{
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        file = fopen(file_path, "a");
        if (file == NULL) {
            perror("fopen");
            return;
        }
    }

    // 写入内容到文件
    fprintf(file, "<html>\r\n");
    fprintf(file, "<head><title>Device Infomation</title></head>\r\n");
    fprintf(file, "<body>\r\n");
    fprintf(file, "%s\r\n", content);
    fprintf(file, "</body>\r\n");
    fprintf(file, "</html>\r\n");

    fclose(file);
}

int send_html_response(int client_socket, const char *file_path)
{
    int   content_length;
    char *html_content = read_html_file(file_path, &content_length);
    if (html_content == NULL) {
        LOGE("Failed to read HTML file.\n");
        return -1;
    }

    char response[MAX_RESPONSE_SIZE];
    snprintf(response, sizeof(response), RESPONSE_TEMPLATE, "html", content_length, html_content);

    // LOGD("content_length: %d\n", content_length);
    // LOGD("response: %s\n", response);

    if (send(client_socket, response, strlen(response), MSG_NOSIGNAL) < 0) {
        perror("send");
        free(html_content);
        return -1;
    }
    free(html_content);

    return 0;
}

int send_plain_response(int client_socket, const char *content)
{
    int  content_length = strlen(content);
    char response[1024] = {0};

    sprintf(response, RESPONSE_TEMPLATE, "plain", content_length, content);

#if DEBUG_HTTP
    LOGD("content_length: %d\n", content_length);
    LOGD("response:\n%s\n", response);
#endif

    if (send(client_socket, response, strlen(response), MSG_NOSIGNAL) < 0) {
        return -1;
    }

    return 0;
}

void get_device_info(char *device_info_content)
{
    g_device_info.running_time = time(NULL) - g_program_start_time;
    strcpy(g_device_info.current_time, GetTimestampString());
    get_sys_mem_payload(&g_device_info.used_ram, &g_device_info.free_ram);

    g_device_info.used_ram_perct =
        100 * g_device_info.used_ram /
        (g_device_info.used_ram + g_device_info.free_ram);

    char running_time[32] = {0};
    format_time(g_device_info.running_time, running_time);

    char temp[1024] = {0};

    sprintf(temp, "device_num:      %s\r\n", g_device_info.device_num);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "ip_addr:         %s\r\n", g_device_info.ip_addr);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "mac_addr:        %s\r\n", g_device_info.mac_addr);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "memory:          %dK used, %dK free, %.2f%% used\r\n", 
            g_device_info.used_ram, 
            g_device_info.free_ram, 
            g_device_info.used_ram_perct);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "app_version:     %s\r\n", g_device_info.app_version);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "vedio_encoder:   %s\r\n", g_device_info.video_encoder);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    if (g_device_info.video_state == COVER_OFF) {
        sprintf(temp, "video_state:     on\r\n");
        strcat(device_info_content, temp);
        temp[0] = '\0';

    } else if (g_device_info.video_state == COVER_ON) {
        sprintf(temp, "video_state:     off\r\n");
        strcat(device_info_content, temp);
        temp[0] = '\0';
    }

    sprintf(temp, "audio_encoder:   %s\r\n", g_device_info.audio_encoder);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "audio_state:     %s\r\n", g_device_info.audio_state);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "start_time:      %s\r\n", g_device_info.start_time);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "current_time:    %s\r\n", g_device_info.current_time);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "running_time:    %s\r\n", running_time);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "push_url:        %s\r\n", g_device_info.push_url);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    sprintf(temp, "server_url:      %s\r\n", g_device_info.server_url);
    strcat(device_info_content, temp);
    temp[0] = '\0';

    // write_html_file(device_info_html, DEVICE_FILE_PATH);
}

void handle_request(int client_socket, char* buf, int buf_size, int* header_size) {
    ssize_t bytes_received;
    char*   request_header = NULL;

    while ((bytes_received = recv(client_socket, buf, buf_size, 0)) > 0) {
        // Append the received data to the request header
        char* new_header = (char*)realloc(request_header, *header_size + bytes_received + 1);
        if (new_header == NULL) {
            fprintf(stderr, "Memory allocation error\n");
            break;
        }

        request_header = new_header;
        memcpy(request_header + *header_size, buf, bytes_received);
        *header_size += bytes_received;
        request_header[*header_size] = '\0';

        // Check if we have received the entire request header
        if (strstr(request_header, "\r\n\r\n") != NULL) {
            break;
        }
    }

    if (bytes_received <= 0) {
        fprintf(stderr, "Failed to read request\n");
        close(client_socket);
        free(request_header);
        return;
    }

    // Print the request header
    // printf("Received Request Header:\n%s\n", request_header);

    memset(buf, 0, buf_size);
    memcpy(buf, request_header, bytes_received);

    // Clean up and close the client socket
    free(request_header);
}

// void* accept_request(void* pclient)
int accept_request(int client)
{
    // int client = *(int*)pclient;

    int  header_size = 0;
    char buf[1024]   = {0};

    char method[256]        = {0};
    char url[256]           = {0};
    char version[256]       = {0};
    char path[256]          = {0};
    char query_string[1024] = {0};

    // 获取一行HTTP请求报文
    header_size = get_request_line(client, buf, sizeof(buf));
    // handle_request(client, buf, sizeof(buf), &header_size);

    // LOGD("http_header: %s\n", buf);

    parse_request_line(buf, header_size, method, url, version);

    // LOGD("url: %s\n", url);

    // printf("method:   %s\n"
    //      "url:      %s\n"
    //      "version:  %s\n", 
    //      method, url, version);

    // 暂时只支持get方法
    // if (strcasecmp(method, "GET")) {
    //     unimplemented(client);
    //     return -1;
    // }

    // 如果是 get 方法，path 可能带 ？参数
    if (strcasecmp(method, "GET") == 0) {
        parse_path_with_params(url, path, query_string);
    } else {
        unimplemented(client);
        return -1;
    }
    // 以上将 request_line 解析完毕

    if (strcasecmp(path, "/hello") == 0) {
        char hello_content[32] = "Hello world!\r\n";
        if (send_plain_response(client, hello_content) != 0) {
            LOGE("send error\n");
            return -1;
        }
        // } else if (strcasecmp(path, "/html") == 0) {
        //     if (send_html_response(client, HTML_FILE_PATH) != 0)
        //         LOGE("send error\n");
    } else if (strcasecmp(path, "/set_params") == 0) {
        LOGD("url: %s\n", url);

        char content[1024] = {0};

        if (deal_query_string(query_string, content) < 0) {
            bad_request(client);
            LOGE("query_string error\n");
            return -1;
        }

        if (send_plain_response(client, content) != 0) {
            LOGE("send error\n");
            return -1;
        }

        if (gs_reboot_switch) {
            gs_reboot_switch = 0;
            LOGI("start rebooting\n");
            reboot_system();
        }

    } else if (strcasecmp(path, "/reboot") == 0) {
        char content[1024] = {0};
        sprintf(content, "Start rebooting\r\n");

        gs_reboot_switch = 1;

        if (send_plain_response(client, content) != 0) {
            LOGE("send device_info error\n");
            return -1;
        }

        if (gs_reboot_switch) {
            gs_reboot_switch = 0;
            LOGI("Start rebooting\n");
            reboot_system();
        }

    } else if (strcmp(path, "/1.jpg") == 0) {
        char jpg_buf[1024 * 500] = {0};
        int  jpg_size            = 0;

        if (LOTO_COMM_VENC_GetSnapJpg(jpg_buf, &jpg_size) == 0) {
            // LOGD("jpg_size:     %d\n", jpg_size);

            send_header(client, "image/jpeg", jpg_size);

            size_t total_len = 0;
            while (total_len < jpg_size) {
                char   buf[1024 * 4] = {0};
                size_t len           = 0;

                len = (total_len + sizeof(buf) <= jpg_size) ? sizeof(buf) : (jpg_size - total_len);
                memcpy(buf, jpg_buf + total_len, len);

                ssize_t send_len = send(client, buf, len, MSG_NOSIGNAL);
                if (send_len <= 0) {
                    LOGD("send jpg error\n");
                    break;
                }
                // usleep(20);
                total_len += send_len;

                // LOGD("total_len: %ld, send_len: %ld\n", total_len, send_len);
            }

            // LOGD("total_len:    %zu\n\n", total_len);

            usleep(1000 * 20);

        } else {
            char response_content[128] = {0};

            LOGE("failed to get snap\r\n");
            sprintf(response_content, "failed to get snap\r\n");

            if (send_plain_response(client, response_content) != 0) {
                LOGE("send error\n");
                return -1;
            }
        }

    } else if ((strcmp(path, "/") == 0) || (strcasecmp(path, "/home") == 0)) {
        char device_info_content[4096] = {0};
        get_device_info(device_info_content);
#if DEBUG_HTTP
        LOGD("device_info_content:\n%s\n", device_info_content);
#endif
        if (send_plain_response(client, device_info_content) != 0) {
            LOGE("send device_info error\n");
            return -1;
        }
    } else {
        not_found(client);
    }

    // close(client);
    // pthread_detach(pthread_self());

    return 0;
}

// socket initial: socket() ---> bind() ---> listen()
int startup(uint16_t *port)
{
    int ret   = 0;
    int httpd = 0;
    int opt   = 0;

    httpd = socket(AF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");

    opt = -1;

    ret = setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (-1 == ret) {
        perror("setsockopt");
        // return -1;
    }

    opt = 1024 * 10;

    ret = setsockopt(httpd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));
    if (-1 == ret) {
        perror("setsockopt");
        // return -1;
    }

    // // 设置Socket为非阻塞模式
    // int flags = fcntl(httpd, F_GETFL, 0);
    // fcntl(httpd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(*port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(httpd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(httpd);
        error_die("bind");
    }

    if (*port == 0) /* Check if the incoming port number is 0.
                       If it is 0, the port needs to be dynamically allocated */
    {
        socklen_t namelen = sizeof(server_addr);
        if (getsockname(httpd, (struct sockaddr *)&server_addr, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(server_addr.sin_port);
    }

    if (listen(httpd, MAX_PENDING) < 0) {
        close(httpd);
        error_die("listen");
    }

    return (httpd);
}

pthread_t accept_thread    = 0;
int       accept_thread_id = 0;

int socket_max  = 10;
int sockets[10] = {0};
int socket_head = 0;
int socket_tail = 0;

void thread_accept_request(void *param) {
    accept_thread_id++;
    while (1) {   
        int socket = sockets[socket_tail % socket_max];
        sockets[socket_tail % socket_max] = 0;

        if (socket != 0) {
            socket_tail++;
            accept_request(socket);
            usleep(1000 * 200);
            close(socket);
        } else {
            usleep(1000);
        }
    }
    accept_thread_id--;
}

void try_accept_request(int socket) {
    if (accept_thread == 0) {
        if (pthread_create(&accept_thread, NULL, (void *)thread_accept_request, NULL) != 0) {
            perror("accept_request");
        }
    }

    if (socket_tail == socket_head) {
        socket_head = socket_tail = 0;
    }

    if (socket_tail + socket_max == socket_head) {
        close(socket);
    } else {
        sockets[socket_head % socket_max] = socket;
        socket_head++;
    }
}

void *http_server(void *arg)
{
    LOGI("====== Start HTTP server ======\n");
    int                server_sock = -1;
    uint16_t           port        = 80; // 监听端口号
    int                client_sock = -1;
    struct sockaddr_in client_name;
    socklen_t          client_name_len = sizeof(client_name);

    server_sock = startup(&port); // 服务器端监听套接字设置
    LOGI("http running on port %d\n", port);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_name, &client_name_len);
        if (client_sock == -1) {
            // error_die("accept");
            close(client_sock);
            continue;
        } else {
            // printf("HTTP client connected.\n");
        }
        // try_accept_request(client_sock);

        accept_request(client_sock);
        usleep(500);
        close(client_sock);
    }

    close(server_sock);

    return 0;
}
