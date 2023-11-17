#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#define TEST_SERVER_URL "http://t.zhuagewawa.com/admin/room/register.pusher"
#define OFFI_SERVER_URL "http://r.zhuagewawa.com/admin/room/register.pusher"

typedef enum CONTROLLER_COVER_STATE {
    COVER_OFF  = 0x00,
    COVER_ON   = 0x01,
    COVER_NULL = 0x0F,
} CONTROLLER_COVER_STATE;

#ifdef __cplusplus
extern "C" {
#endif

void* http_server(void* arg);

#ifdef __cplusplus
}
#endif

#endif