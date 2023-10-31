#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>

#include <errno.h>
#include <net/if.h>
#include <netdb.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/timeb.h>

#include "xiecc_rtmp.h"
#include <inttypes.h>
#include "common.h"
#include "log.h"
#include "ConfigParser.h"

#define LOTO_DBG(s32Ret)\
    do{\
        printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
    }while(0)


#define MAX_FRAME_COUNT     30


// int     shmid;
// void*   pshm;
char    sz_pushurl[1024];
HI_U64	v_s_timestamp = 0;
HI_U64	a_s_timestamp = 0;
uint32_t start_time = 0;

pthread_t vid = 0, aid = 0;
static  int isopen = 1;

int i_algorithm = 0;
int i_with_audio = 0;


int stat_count[32] = {0};

#define BUFFER_SIZE     2*1024*1024

unsigned char   full_h264[BUFFER_SIZE];
size_t      read_count = 0;


void read_data_fromfile()
{
    FILE *fp;
    int  flag = 0;
    char sTitle[32], *wTmp;
    static char sLine[1024];

    fp = fopen("stream_chn0.h264", "r");
    if (fp != NULL)
    {
        read_count = fread(full_h264, sizeof(unsigned char), BUFFER_SIZE, fp);
        fclose(fp);
    }

}


static uint8_t *get_nal(uint32_t *len, uint8_t **offset, uint8_t *start, uint32_t total)
{
    uint32_t info;
    uint8_t *q;
    uint8_t *p = *offset;
    *len = 0;

    if ((p - start) >= total)
        return NULL;

    while (1)
    {
        info = find_start_code(p, 3);
        if (info == 1)
            break;
        p++;
        if ((p - start) >= total)
            return NULL;
    }
    q = p + 4;
    p = q;
    while (1)
    {
        info = find_start_code(p, 3);
        if (info == 1)
            break;
        p++;
        if ((p - start) >= total)
            break;
    }

    *len = (p - q);
    *offset = p;
    return q;
}

void *gs_rtmp = NULL;

void* LOTO_VIDEO_AUDIO_RTMP(void *p)
{
    uint64_t v_timeCount = 0;

    long long timestamp = 0;
    long long cur_time = 0;
    long long v_start_time = 0;

    long v_count = 0;

    struct ringbuf v_ringinfo;
    int v_ringbuflen=0;

    struct ringbuf a_ringinfo;
    int a_ringbuflen=0;

    long long t1 = 0, t2 = 0;
    int i = 0;

    LOGD("[%s] LOTO_VIDEO_AUDIO_RTMP \n", log_Time());

    while(1)
	{
        cur_time = get_timestamp(NULL, 1);
		v_ringbuflen = ringget(&v_ringinfo);
        if(v_ringbuflen > 0)
		{
            //timestamp = cur_time; //v_ringinfo.timestamp / 1000;
            if (v_start_time == 0) {
            {
                v_timeCount = 0;
                v_start_time = cur_time;
            }
                
            v_timeCount += 20;
  
            t1 = get_us_timestamp();
            if (gs_rtmp != NULL)
			    rtmp_sender_write_avc_frame(gs_rtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount, 0);
            t2 = get_us_timestamp();

            int offset = (int)((t2 - t1)/1000);
            
            if (offset == 0) {
                stat_count[0]++;
            } else if (offset == 1) {
                stat_count[1]++;
            } else if (offset >= 5 * 10) {
                stat_count[12]++;
            } else {
                stat_count[offset / 5 + 2]++;
            }
            v_count ++;
            if (v_count == 3000)
            {
                v_count = 0;
                LOGD("[%s] rtmp_write_video: v_timeCount = %"PRIu64"", log_Time(), v_timeCount);
                char s_print[512] = "";
                sprintf(s_print, "%d", stat_count[0]);
                stat_count[0] = 0;
                for (i = 1; i < 13; i ++)
                {
                    sprintf(s_print, "%s, %d", s_print, stat_count[i]);
                    stat_count[i] = 0;
                }
                LOGD("[%s] rtmp_write_video: statistics write time = %s", log_Time(), s_print);
            }
		}

        usleep(1000);
	}
}

int main(int argc, char *argv[])
{
	pthread_t rtmp_pid;
    pthread_t sync_pid;

    InitTCpRtpLog();

	if(argc!=2)
	{
		// printf("Usage: rtmp url -eg<< rtmp://push.zhuagewawa.com/record/w054?wsSecret=099122fec49ef6a80bf58d7147f0d39c&wsABSTime=1677808601 >>\n");
        LOGD ("[%s] Usage: rtmp url -eg<< rtmp://push.zhuagewawa.com/record/w054?wsSecret=099122fec49ef6a80bf58d7147f0d39c&wsABSTime=1677808601 >>\n", log_Time());
		return -1;
	}

    char szServerFile[256] = "";
	sprintf(szServerFile, "%s/server.ini", WORK_FOLDER);

	sprintf(sz_pushurl, argv[1]);

    read_data_fromfile();

    if (read_count > 0)
    {
        prtmp = rtmp_sender_alloc(sz_pushurl); //return handle
        if(rtmp_sender_start_publish(gs_rtmp, 0, 0)!=0)
        {
            printf("connect %s fail\n", sz_pushurl);
            LOGD ("[%s] connect %s fail\n", log_Time(), sz_pushurl);
            rtmp_sender_free(gs_rtmp);
            gs_rtmp = NULL;
            return -1;
        }

        pthread_create(&rtmp_pid, NULL, LOTO_VIDEO_AUDIO_RTMP, NULL);

        while(1)
        {	
            sleep(1);
        }
    }

	
	printf("end\n");
	return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */