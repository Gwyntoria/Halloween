/*
 * Common.h:
 *
 ***********************************************************************
 * by Jessica Mao
 * Lotogram Inc,. 2020/05/18
 *
 ***********************************************************************
 */

#ifndef COMMON_H
#define COMMON_H

#include "cJSON.h"
#include "ConfigParser.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WORK_FOLDER     "/home/pi/wawaji/"

// LOG system
#define LOG_LEVEL 0x0F

#define LOG_LVL_ERROR 0x01
#define LOG_LVL_WARN  0x02
#define LOG_LVL_INFO  0x04
#define LOG_LVL_DEBUG 0x08

#if (LOG_LEVEL & LOG_LVL_INFO)
#define LOGI(format, ...) \
    do { \
        fprintf(stderr, "INFO--%s -%4d " format "\n", __FILE__, __LINE__, ##__VA_ARGS__ ); \
    } while (0)
#else
#define LOGI(format, ...) \
    do { } while (0)
#endif

#if (LOG_LEVEL & LOG_LVL_ERROR)
#define LOGE(format, ...) \
    do { \
        fprintf(stderr, "ERROR--%s -%4d " format "\n", __FILE__, __LINE__, ##__VA_ARGS__ ); \
    } while (0)
#else
#define LOGE(format, ...) \
    do { } while (0)
#endif

#if (LOG_LEVEL & LOG_LVL_WARN)
#define LOGW(format, ...) \
    do { \
        fprintf(stderr, "WARNING--%s -%4d " format "\n", __FILE__, __LINE__, ##__VA_ARGS__ ); \
    } while (0)
#else
#define LOGW(format, ...) \
    do { } while (0)
#endif

#if (LOG_LEVEL & LOG_LVL_DEBUG)
#define LOGD(format, ...) \
    do { \
        fprintf(stderr, "DEBUG--%s -%4d " format "\n", __FILE__, __LINE__, ##__VA_ARGS__ ); \
    } while (0)
#else
#define LOGD(format, ...) \
    do { } while (0)
#endif

typedef struct LOTO_ROOM_INFO
{
    char	szStatus[32];
    int 	iCode;

    int		iRoomType;
    int 	iSubType;
    int		iVip;
    int		iJP;
    int		iMatchType;
    int		iEncodeType;
    int 	iPlayerCount;
    int     iSupportMonster;

    char	szRoomId[64];
    char	szDomain[32];
    char	szName[32];
    char	szHostName[32];
    char	szRouterIp[32];
    char	szWebsocket[64];
    char	szIp[32];
    char	szRouterDNS[128];
    char	szFrpPort[32];
    char    szFrps[128];
    char	szPushURL[256];
    char	szRTMPIp[32];
    char    szScreenShot[256];

}loto_room_info;


char*   log_Time(void);
int     get_hash_code_24(char* psz_combined_string);
int     get_mac(char* mac);
int     int2string(long long value, char * output);
long long string2int(const char *str);
long long get_timestamp(char* pszTS, int isMSec);


#ifdef __cplusplus
}
#endif

#endif