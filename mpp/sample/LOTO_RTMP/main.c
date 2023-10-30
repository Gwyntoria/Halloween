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

#include "ringfifo.h"
#include "sample_comm.h"
#include "xiecc_rtmp.h"
#include <inttypes.h>
#include "common.h"
#include "log.h"
#include "EasyAACEncoderAPI.h"
#include "ConfigParser.h"

#include "faac.h"

#define LOTO_DBG(s32Ret)\
    do{\
        printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
    }while(0)


#define MAX_FRAME_COUNT     30

typedef unsigned long ULONG;
typedef unsigned int UINT;
typedef unsigned char BYTE;


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

extern void * LOTO_AENC_AAC_CLASSIC(void *p);
extern void* LOTO_VENC_CLASSIC(void *p);
extern HI_S32 LOTO_AUDIO_CreatTrdAenc(AENC_CHN AeChn, pthread_t* aencPid);
extern HI_S32 LOTO_AUDIO_DestoryTrdAenc(AENC_CHN AeChn);
extern HI_S32 LOTO_AUDIO_CreatTrdAi(AI_CHN AiChn, pthread_t* aiPid);
extern HI_S32 LOTO_COMM_VENC_StartGetStream(HI_S32 s32Cnt, pthread_t* vencPid);
// extern HI_S32 LOTO_VENC_DisplayCover();
// extern HI_S32 LOTO_VENC_RemoveCover();
Easy_Handle g_Easy_H = NULL;

int stat_count[32] = {0};

int is_rtmp_write = 0;


HI_S32	LOTO_RTMP_VA_CLASSIC()
{
    SAMPLE_VI_MODE_E enViMode      = SAMPLE_VI_MODE_1_720P;
    VIDEO_NORM_E     gs_enNorm     = VIDEO_ENCODING_MODE_PAL;
    HI_U32           u32ViChnCnt   = 1;
    HI_S32           s32VpssGrpCnt = 1;
    PAYLOAD_TYPE_E   enPayLoad     = PT_H264;

    VPSS_GRP        VpssGrp = 0;
    VPSS_GRP_ATTR_S stGrpAttr;
    VENC_GRP        VencGrp = 0;
    VENC_CHN        VencChn = 0;
    SAMPLE_RC_E     enRcMode;

    SIZE_S stSize;

    VB_CONF_S stVbConf;
    HI_U32    u32BlkSize;

    AUDIO_DEV  AiDev = 0;
    HI_S32     i, s32Ret = HI_SUCCESS;
    AIO_ATTR_S stAioAttr;
    HI_S32     s32AiChnCnt;
    HI_S32     s32AencChnCnt;
    AENC_CHN   AeChn;
    AI_CHN     AiChn;
    pthread_t  aenc_Pid;
    pthread_t  venc_Pid;

    InitParam initParam;

    /******************************************
     step  1: init global  variable 
    ******************************************/   
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, PIC_HD720, SAMPLE_PIXEL_FORMAT, 
                                                  SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;

    /* video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 10;
    memset(stVbConf.astCommPool[0].acMmzName,0,
        sizeof(stVbConf.astCommPool[0].acMmzName));

    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1HD_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1HD_0;
    }
    
    /******************************************
     step 4: start vpss and vi bind vpss (subchn needn't bind vpss in this mode)
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_HD720, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1HD_0;
    }

    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_FALSE;//HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;

    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,&stGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_1HD_1;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_1HD_2;
    }

    enRcMode = SAMPLE_RC_VBR;

	/******************************************
     step 6: start stream venc (big + little)
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad,\
                                    gs_enNorm, PIC_HD720, enRcMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1HD_3;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VPSS_BSTR_CHN);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1HD_3;
    }

    /******************************************
     step 7: stream venc process -- get stream, then save it to file. 
    ******************************************/
    s32Ret = LOTO_COMM_VENC_StartGetStream(u32ViChnCnt, &venc_Pid);
    // s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ViChnCnt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1HD_3;
    }

    // pthread_join(venc_Pid, 0);

    stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_48000;
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 5;
    stAioAttr.u32PtNumPerFrm = 1024;
    stAioAttr.u32ChnCnt = 2;
    stAioAttr.u32ClkSel = 0;

    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32AencChnCnt = s32AiChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, HI_FALSE, NULL);
    if (s32Ret != HI_SUCCESS)
    {
        LOTO_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 3: start Aenc
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, PT_LPCM);
    if (s32Ret != HI_SUCCESS)
    {
        LOTO_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 4: Aenc bind Ai Chn
    ********************************************/
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        // printf("Ai(%d,%d) bind to AencChn:%d begin!\n",AiDev , AiChn, AeChn);

        s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
        if (s32Ret != HI_SUCCESS)
        {
            LOTO_DBG(s32Ret);
            return s32Ret;
        }
        // printf("Ai(%d,%d) bind to AencChn:%d ok!\n",AiDev , AiChn, AeChn);
    }

	// initParam.u32AudioSamplerate=44100;
	// initParam.ucAudioChannel=1;
	// initParam.u32PCMBitSize=16;
	// initParam.ucAudioCodec = Law_PCM16;

	// g_Easy_H = Easy_AACEncoder_Init(initParam);

    s32Ret = LOTO_AUDIO_CreatTrdAenc(0, NULL);
    // s32Ret = LOTO_AUDIO_CreatTrdAi(0, &aenc_Pid);
    if (s32Ret != HI_SUCCESS)
    {
        LOTO_DBG(s32Ret);
        return HI_FAILURE;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /********************************************
      step 6: exit the process
    ********************************************/

    LOTO_AUDIO_DestoryTrdAenc(AeChn);

    Easy_AACEncoder_Release(g_Easy_H);
    
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;
        SAMPLE_COMM_AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
    }
    
    SAMPLE_COMM_AUDIO_StopAenc(s32AencChnCnt);
    SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, HI_FALSE, HI_FALSE);

    /******************************************
     step 8: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1HD_3:
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VPSS_BSTR_CHN);
    SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1HD_2:	//vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_1HD_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);

    // pthread_create(&vid, NULL, LOTO_VENC_CLASSIC, NULL);
    // pthread_create(&aid, NULL, LOTO_AENC_AAC_CLASSIC, NULL);

END_VENC_1HD_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return HI_SUCCESS;
}

/**************************************************************************************************
**
**
**
**************************************************************************************************/

void *prtmp = NULL;

void* LOTO_VIDEO_AUDIO_RTMP(void *p)
{
    uint64_t v_timeCount = 0;
    uint64_t a_timeCount = 0;

    long long timestamp = 0;
    long long cur_time = 0;
    long long a_start_time = 0;
    long long v_start_time = 0;

    long long begin_time = 0;
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
        begin_time = get_timestamp(NULL, 1);
        // if (prtmp != NULL && !rtmp_sender_isOK(prtmp))
        // {
        //     LOGE("[%s] rtmp is disconnected \n", log_Time());
        //     rtmp_sender_stop_publish(prtmp);
        //     rtmp_sender_free(prtmp);
        //     sleep(1);
        //     prtmp = rtmp_sender_alloc(sz_pushurl);
        //     if (rtmp_sender_start_publish(prtmp, 0, 0) != 0)
        //     {
        //         LOGE("[%s] connect %s fail \n", log_Time(), sz_pushurl);
        //     }
        // }
        cur_time = get_timestamp(NULL, 1);
		a_ringbuflen = ringget_audio(&a_ringinfo);
        if (i_with_audio == 1 && a_ringbuflen > 0)
		{   
            //timestamp = cur_time; //a_ringinfo.timestamp / 1000;
            if (a_start_time == 0) {
                a_start_time = cur_time;
                v_start_time = cur_time;
            }
            a_timeCount = cur_time - a_start_time;
            // LOGD("[%s] rtmp_sender_write_audio_frame: a_ringinfo.timestamp = %"PRIu64", a_timeCount = %"PRIu64" \n ", log_Time(), a_ringinfo.timestamp, a_timeCount);
            // LOGD("[%s] rtmp_write_audio: a_timeCount = %"PRIu64" \n ", log_Time(), a_timeCount);
            if (prtmp != NULL)
			    rtmp_sender_write_aac_frame(prtmp, a_ringinfo.buffer, a_ringinfo.size, a_timeCount, 0);
		}

        cur_time = get_timestamp(NULL, 1);
		v_ringbuflen = ringget(&v_ringinfo);
        if(v_ringbuflen > 0)
		{
            //timestamp = cur_time; //v_ringinfo.timestamp / 1000;
            if (v_start_time == 0) {
                v_start_time = cur_time;
                a_start_time = cur_time;
            }
            v_timeCount = cur_time - v_start_time;
            // LOGD("[%s] rtmp_sender_write_video_frame: v_ringinfo.timestamp = %"PRIu64", v_timeCount = %"PRIu64" \n ", log_Time(), v_ringinfo.timestamp, v_timeCount);
            // LOGD("[%s] rtmp_write_video:  v_timeCount = %"PRIu64" \n", log_Time(), v_timeCount);

            // if (a_ringbuflen > 0)
            //     usleep(200);

            is_rtmp_write = 1;
            t1 = get_timestamp_us();
            if (prtmp != NULL)
			    rtmp_sender_write_avc_frame(prtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount, 0);
            t2 = get_timestamp_us();
            is_rtmp_write = 0;

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
        // long long consume_time = get_timestamp(NULL, 1) - begin_time;
        // if (consume_time < 2) {
        //     usleep(500);
        // }

        usleep(1000);

		// if (v_ringbuflen + a_ringbuflen <= 0)
		// 	usleep(1);
        // else
        //     usleep(1000);
	}
}

void* LOTO_VIDEO_AUDIO_RTMP_1(void *p)
{

    uint64_t last_timeCount = 0;

    int a_offset = 22;
    int v_offset = 34;

    bool a_writed = true;
    bool v_writed = true;

    int n_v_count = 0;
    int v_count   = 0;

    uint64_t start_time   = 0;
    uint64_t a_start_time = 0;
    uint64_t v_start_time = 0;
    uint64_t a_current_time  = 0;
    uint64_t v_current_time  = 0;

    struct ringbuf a_ringinfo   = {0};
    int            a_ringbuflen = 0;

    struct ringbuf v_ringinfo   = {0};
    int            v_ringbuflen = 0;

    uint64_t t1 = 0, t2 = 0;

    LOGD("[%s] LOTO_VIDEO_AUDIO_RTMP_1 \n", log_Time());

    while(1)
	{
        // 判断是否可进行推流
        if (a_writed == true && i_with_audio == 1)
        {
            a_ringbuflen = ringget_audio(&a_ringinfo);
            // LOGD("[%s] LOTO_VIDEO_AUDIO_RTMP_1, a_ringbuflen = %d", log_Time(), a_ringbuflen);
            if (a_ringbuflen > 0) {
                a_current_time = get_timestamp(NULL, 1);

                if (start_time == 0) {
                    start_time = a_current_time;
                }

                a_writed = false;

                if (prtmp != NULL) {
                    uint64_t a_timestamp = a_current_time - start_time;
                    rtmp_sender_write_aac_frame(prtmp, a_ringinfo.buffer,
                                                a_ringinfo.size, a_timestamp,
                                                a_start_time);

                    // printf("aac timestap: %llu\n", a_timestamp);

                    a_writed = true;
                }
            }
        }
        else
        {
            // LOGD("[%s] LOTO_VIDEO_AUDIO_RTMP_1, cant get audio", log_Time());
        }
		
        if (v_writed == true)
        {
            v_ringbuflen = ringget(&v_ringinfo);
            // LOGD("[%s] LOTO_VIDEO_AUDIO_RTMP_1, v_ringbuflen = %d", log_Time(), v_ringbuflen);
            if (v_ringbuflen > 0) {
                v_current_time = get_timestamp(NULL, 1);

                if (start_time == 0) {
                    start_time = v_current_time;
                }

                v_writed = false;

                if (prtmp != NULL) {
                    uint64_t v_timestamp = v_current_time - start_time;
                    rtmp_sender_write_avc_frame(prtmp, v_ringinfo.buffer,
                                                v_ringinfo.size, v_timestamp,
                                                0);

                    // printf("264 timestap: %llu\n", v_timestamp);

                    v_writed = true;
                }
            }
        }
        else
        {
            // LOGD("[%s] LOTO_VIDEO_AUDIO_RTMP_1, cant get video", log_Time());
        }

        t1 = 0;
        t2 = 0;

        // 已经获取到音视频帧
        if (a_ringbuflen > 0 || v_ringbuflen > 0)
        {
           /*  if (start_time == 0) {
                if (a_ringbuflen > 0 && v_ringbuflen > 0)
                {
                    start_time = a_timeCount < v_timeCount ? a_timeCount : v_timeCount;

                    if (a_timeCount <= v_timeCount)
                    {
                        start_time = a_timeCount;
                        if (prtmp != NULL)
			                rtmp_sender_write_aac_frame(prtmp, a_ringinfo.buffer, a_ringinfo.size, 0, start_time);
                        a_writed = true;
                        last_timeCount = 0;
                        a_ringbuflen = 0;
                        // LOGD("[%s] rtmp_write_audio_0:  last_timeCount = %"PRIu64", audio timestamp = %"PRIu64"", log_Time(), last_timeCount, a_ringinfo.timestamp / 1000);

                        if ((a_timeCount + a_offset) > v_timeCount)
                        {
                            is_rtmp_write = 1;
                            t1 = get_timestamp_us();
                            if (prtmp != NULL)
			                    rtmp_sender_write_avc_frame(prtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount - start_time, 0);
                            t2 = get_timestamp_us();
                            is_rtmp_write = 0;
                            v_writed = true;
                            n_v_count ++;
                            v_count ++;
                            last_timeCount = v_timeCount - start_time;
                            v_ringbuflen = 0;
                            // LOGD("[%s] rtmp_write_video_0:  last_timeCount = %"PRIu64", video timestamp = %"PRIu64"", log_Time(), last_timeCount, v_ringinfo.timestamp / 1000);
                        }
                    }
                    else
                    {
                        start_time = v_timeCount;
                        is_rtmp_write = 1;
                        t1 = get_timestamp_us();
                        if (prtmp != NULL)
			                rtmp_sender_write_avc_frame(prtmp, v_ringinfo.buffer, v_ringinfo.size, 0, 0);
                        t2 = get_timestamp_us();
                        is_rtmp_write = 0;
                        v_writed = true;
                        n_v_count ++;
                        v_count ++;
                        last_timeCount = 0;
                        v_ringbuflen = 0;
                        // LOGD("[%s] rtmp_write_video_0_1:  last_timeCount = %"PRIu64", video timestamp = %"PRIu64"", log_Time(), last_timeCount, v_ringinfo.timestamp / 1000);

                        if ((v_timeCount + v_offset) > a_timeCount)
                        {
                            if (prtmp != NULL)
			                    rtmp_sender_write_aac_frame(prtmp, a_ringinfo.buffer, a_ringinfo.size, a_timeCount-start_time, start_time);
                            a_writed = true;
                            last_timeCount = a_timeCount-start_time;
                            a_ringbuflen = 0;
                            // LOGD("[%s] rtmp_write_audio_0_1:  last_timeCount = %"PRIu64", audio timestamp = %"PRIu64"", log_Time(), last_timeCount, a_ringinfo.timestamp / 1000);
                        }
                    }   

                }
                else if (a_ringbuflen > 0)
                {
                    start_time = a_timeCount;
                    if (prtmp != NULL)
			            rtmp_sender_write_aac_frame(prtmp, a_ringinfo.buffer, a_ringinfo.size, 0, a_ringinfo.timestamp - start_time);
                    a_writed = true;
                    last_timeCount = 0;
                    a_ringbuflen = 0;
                    // LOGD("[%s] rtmp_write_audio_0_2:  last_timeCount = %"PRIu64", audio timestamp = %"PRIu64"", log_Time(), last_timeCount, a_ringinfo.timestamp / 1000);
                }   
                else if (v_ringbuflen > 0)
                {
                    start_time = v_timeCount;
                    is_rtmp_write = 1;
                    t1 = get_timestamp_us();
                    if (prtmp != NULL)
			            rtmp_sender_write_avc_frame(prtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount - start_time, 0);
                    t2 = get_timestamp_us();
                    is_rtmp_write = 0;
                    v_writed = true;
                    n_v_count ++;
                    v_count ++;
                    last_timeCount = 0;
                    v_ringbuflen = 0;
                    // LOGD("[%s] rtmp_write_video_0_2:  last_timeCount = %"PRIu64", video timestamp = %"PRIu64"", log_Time(), last_timeCount, v_ringinfo.timestamp / 1000);
                }
            } else {
                if (a_ringbuflen > 0 && v_ringbuflen > 0)
                {
                    if (a_timeCount <= v_timeCount)
                    {
                        if (prtmp != NULL)
			                rtmp_sender_write_aac_frame(prtmp, a_ringinfo.buffer, a_ringinfo.size, a_timeCount-start_time, start_time);
                        a_writed = true;
                        last_timeCount = a_timeCount-start_time;
                        a_ringbuflen = 0;
                        // LOGD("[%s] rtmp_write_audio_1:  last_timeCount = %"PRIu64", audio timestamp = %"PRIu64"", log_Time(), last_timeCount, a_ringinfo.timestamp / 1000);

                        if ((a_timeCount + a_offset) > v_timeCount)
                        {
                            is_rtmp_write = 1;
                            t1 = get_timestamp_us();
                            if (prtmp != NULL)
			                    rtmp_sender_write_avc_frame(prtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount-start_time, 0);
                            t2 = get_timestamp_us();
                            is_rtmp_write = 0;
                            v_writed = true;
                            last_timeCount = v_timeCount - start_time;
                            v_ringbuflen = 0;
                            
                            if (n_v_count == 5)
                            {
                                v_offset = 34;
                                n_v_count = 0;
                            }
                            else
                                v_offset = 17;
                            n_v_count ++;
                        }
                    }
                    else
                    {
                        t1 = get_timestamp_us();
                        if (v_timeCount < start_time)
                            v_timeCount = start_time;
                        is_rtmp_write = 1;
                        if (prtmp != NULL)
			                rtmp_sender_write_avc_frame(prtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount-start_time, 0);
                        t2 = get_timestamp_us();
                        is_rtmp_write = 0;

                        v_writed = true;
                        last_timeCount = v_timeCount - start_time;
                        v_ringbuflen = 0;

                        if (n_v_count == 5)
                        {
                            v_offset = 34;
                            n_v_count = 0;
                        }
                        else
                            v_offset = 17;
                        n_v_count ++;

                        if ((v_timeCount + v_offset) > a_timeCount)
                        {
                            if (prtmp != NULL)
			                    rtmp_sender_write_aac_frame(prtmp, a_ringinfo.buffer, a_ringinfo.size, a_timeCount-start_time, start_time);
                            a_writed = true;
                            last_timeCount = a_timeCount-start_time;
                            a_ringbuflen = 0;
                            // LOGD("[%s] rtmp_write_audio_1_1:  last_timeCount = %"PRIu64"", log_Time(), last_timeCount);
                        }
                    }   

                }
                else if (a_ringbuflen > 0)
                {
                    if ((last_timeCount + v_offset) >= a_timeCount)
                    {
                        if (prtmp != NULL)
                            rtmp_sender_write_aac_frame(prtmp, a_ringinfo.buffer, a_ringinfo.size, a_timeCount-start_time, start_time);
                        a_writed = true;
                        last_timeCount = a_timeCount-start_time;
                        a_ringbuflen = 0;
                        // LOGD("[%s] rtmp_write_audio_1_2:  last_timeCount = %"PRIu64"", log_Time(), last_timeCount);
                    }
                }   
                else if (v_ringbuflen > 0)
                {
                    if (i_with_audio == 0 || ((last_timeCount + a_offset) >= v_timeCount))
                    {
                        is_rtmp_write = 1;
                        t1 = get_timestamp_us();
                        if (prtmp != NULL)
                            rtmp_sender_write_avc_frame(prtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount-start_time, 0);
                        t2 = get_timestamp_us();
                        is_rtmp_write = 0;
                        v_writed = true;
                        last_timeCount = v_timeCount - start_time;
                        v_ringbuflen = 0;

                        if (n_v_count == 5)
                        {
                            v_offset = 34;
                            n_v_count = 0;
                        }
                        else
                            v_offset = 17;
                        n_v_count ++;
                    }
                }
            }
 */

            /* Calculate the distribution of streaming duration for each frame */
           /*  if (t1 != 0 && t2 != 0)
            {
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

                if (v_count == 0 || v_count == 3000)
                {
                    // LOGD("[%s] rtmp_write_video:  last_timeCount = %"PRIu64", video timestamp = %"PRIu64"", log_Time(), last_timeCount, v_ringinfo.timestamp / 1000);

                    int i = 0;
                    char s_print[512] = "";
                    sprintf(s_print, "%d", stat_count[0]);
                    stat_count[0] = 0;
                    for (i = 1; i < 13; i ++)
                    {
                        sprintf(s_print, "%s, %d", s_print, stat_count[i]);
                        stat_count[i] = 0;
                    }
                    // LOGD("[%s] rtmp_write_video: statistics write time = %s", log_Time(), s_print);

                    v_count = 0;
                }
                v_count ++;
            } */
            
        }
        else
        {
            a_writed = true;
            v_writed = true;
        }

        a_ringbuflen = 0;
        v_ringbuflen = 0;
        usleep(500);
	}
}

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
        LOGE("create socket failed\n");
        return -1;
    }

    // Set socket to non-blocking mode
    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

    LOGI("create udp_socket success!\n");

    struct hostent* server = gethostbyname(ntp_server);
    if (server == NULL) {
        LOGE("could not resolve %s\n", ntp_server);
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
            LOGE("sendto failed\n");
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
            LOGE("select error");
            return -1;
        } else if (ret == 0) {
            LOGE("timeout, retry.....\n");
            retries++;
            continue;
        }

        memset(ntp_response, 0, sizeof(ntp_response));
        ssize_t bytes_received
            = recvfrom(sock_fd, ntp_response, sizeof(ntp_response), 0, (struct sockaddr*)&server_addr, &addr_len);
        if (bytes_received < 0) {
            if (errno == EINTR) {
                LOGE("No data available; retrying...\n");
                retries++;
                continue;
            } else {
                LOGE("recv error; retrying...\n");
                retries++;
                continue;
            }
        }

        LOGI("receive ntp_packet success!\n");
        break;
    }

    close(sock_fd);

    if (retries == MAX_RETRIES) {
        LOGI("Maximum number of retries reached; giving up.\n");
        return -1;
    }

    uint32_t ntp_time  = ntohl(*(uint32_t*)(ntp_response + 40));
    time_t   unix_time = ntp_time - NTP_UNIX_OFFSET;

    if (settimeofday(&(struct timeval){.tv_sec = unix_time}, NULL) == -1) {
        LOGE("settimeofday failed\n");
        return -1;
    }

    LOGI("Time from %s: %s", ntp_server, ctime(&unix_time));

    return 0;
}

void* SYNC_TIME(void *p)
{
    while (1)
    {
        sleep(60);
        get_net_time();
    }
    return  NULL;
}

int main(int argc, char *argv[])
{
	pthread_t rtmp_pid;
    pthread_t sync_pid;

    /* Initialize rtmp_log file */
    // FILE *rtmp_log = InitRtmpLogFile();
    // if (rtmp_log == NULL)
    // {
    //     LOGE("create rtmp_log failed! \n");
    // }
    // else
    //     RTMP_LogSetOutput(rtmp_log);

    InitTCpRtpLog();

    if (argc != 2) {
        // printf("Usage: rtmp url -eg<<
        // rtmp://push.zhuagewawa.com/record/w054?wsSecret=099122fec49ef6a80bf58d7147f0d39c&wsABSTime=1677808601
        // >>\n");
        LOGD("[%s] Usage: rtmp url -eg<< "
             "rtmp://push.zhuagewawa.com/record/"
             "w054?wsSecret=099122fec49ef6a80bf58d7147f0d39c&wsABSTime="
             "1677808601 >>\n",
             log_Time());
        return -1;
    }

    char szServerFile[256] = "";
	sprintf(szServerFile, "%s/server.ini", WORK_FOLDER);

    i_algorithm = GetIniKeyInt((char*)"server", (char*)"push_algorithm", szServerFile);
    i_with_audio = GetIniKeyInt((char*)"server", (char*)"with_audio", szServerFile);

	sprintf(sz_pushurl, argv[1]);

	ringmalloc(500*1024);
	ringmalloc_audio(1024*5);
	prtmp = rtmp_sender_alloc(sz_pushurl); //return handle
    if(rtmp_sender_start_publish(prtmp, 0, 0)!=0)
	{
		printf("connect %s fail\n", sz_pushurl);
        LOGD ("[%s] connect %s fail\n", log_Time(), sz_pushurl);
        rtmp_sender_free(prtmp);
        prtmp = NULL;
		return -1;
	}

    if (i_algorithm == 0)
        pthread_create(&rtmp_pid, NULL, LOTO_VIDEO_AUDIO_RTMP, NULL);
    else if (i_algorithm == 1)
        pthread_create(&rtmp_pid, NULL, LOTO_VIDEO_AUDIO_RTMP_1, NULL);
    // pthread_create(&sync_pid, NULL, SYNC_TIME, NULL);
	LOTO_RTMP_VA_CLASSIC();

	while(1)
	{	
		sleep(1);
	}
	printf("end\n");
	return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */