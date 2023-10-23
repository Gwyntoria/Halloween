// #ifdef __cplusplus
// #if __cplusplus
// extern "C"{
// #endif
// #endif /* End of #ifdef __cplusplus */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


#define NTP_PORT        123
#define NTP_PACKET_SIZE 48
#define NTP_UNIX_OFFSET 2208988800
#define RESEND_INTERVAL 3
#define MAX_RETRIES     5
#define TIMEOUT_SEC     5

int get_net_time() {
    char* ntp_server = "ntp1.aliyun.com";

    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd == -1) {
        // LOGE("create socket failed\n");
        return -1;
    }

    // Set socket to non-blocking mode
    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

    // LOGI("create udp_socket success!\n");

    struct hostent* server = gethostbyname(ntp_server);
    if (server == NULL) {
        // LOGE("could not resolve %s\n", ntp_server);
        return -1;
    }

    // Populate the server information
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(NTP_PORT);

    // Construct NTP packets
    uint8_t ntp_packet[NTP_PACKET_SIZE];
    memset(ntp_packet, 0, sizeof(ntp_packet));
    ntp_packet[0] = 0b11100011; // NTP version 4, client mode, no leap indicator
    ntp_packet[1] = 0;          // stratum, or how far away the server is from a reliable time source
    ntp_packet[2] = 6;          // poll interval, or how often the client will request time
    ntp_packet[3] = 0xEC;       // precision, or how accurate the client's clock is
    // the rest of the packet is all zeros

    int       retries = 0;
    uint8_t   ntp_response[NTP_PACKET_SIZE];
    socklen_t addr_len = sizeof(server_addr);

    while (retries < MAX_RETRIES) {
        ssize_t bytes_sent
            = sendto(sock_fd, ntp_packet, sizeof(ntp_packet), 0, (struct sockaddr*)&server_addr, addr_len);
        if (bytes_sent < 0) {
            // LOGE("sendto failed\n");
            break;
        }

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock_fd, &read_fds);

        // Waiting for data return or timeout
        struct timeval timeout;
        timeout.tv_sec  = TIMEOUT_SEC;
        timeout.tv_usec = 0;

        int ret = select(sock_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (ret == -1) {
            // LOGE("select error");
            return -1;
        } else if (ret == 0) {
            // LOGE("timeout, retry.....\n");
            retries++;
            continue;
        }

        memset(ntp_response, 0, sizeof(ntp_response));
        ssize_t bytes_received
            = recvfrom(sock_fd, ntp_response, sizeof(ntp_response), 0, (struct sockaddr*)&server_addr, &addr_len);
        if (bytes_received < 0) {
            if (errno == EINTR) {
                // LOGE("No data available; retrying...\n");
                retries++;
                continue;
            } else {
                // LOGE("recv error; retrying...\n");
                retries++;
                continue;
            }
        }

        // LOGI("receive ntp_packet success!\n");
        break;
    }

    close(sock_fd);

    if (retries == MAX_RETRIES) {
        // LOGI("Maximum number of retries reached; giving up.\n");
        return -1;
    }

    uint32_t ntp_time  = ntohl(*(uint32_t*)(ntp_response + 40));
    time_t   unix_time = ntp_time - NTP_UNIX_OFFSET;

    if (settimeofday(&(struct timeval){.tv_sec = unix_time}, NULL) == -1) {
        // LOGE("settimeofday failed\n");
        return -1;
    }

    // LOGI("Time from %s: %s", ntp_server, ctime(&unix_time));

    return 0;
}

int main(int argc, char *argv[])
{
    while (1)
    {
        get_net_time();
        sleep(60);
    }
    
    
	return 0;
}

// #ifdef __cplusplus
// #if __cplusplus
// }
// #endif
// #endif /* End of #ifdef __cplusplus */