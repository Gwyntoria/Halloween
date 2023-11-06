#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "faac.h"
#include "log.h"
#include "sample_comm.h"
#include "xiecc_rtmp.h"

#include "ConfigParser.h"
#include "Loto_aenc.h"
#include "Loto_venc.h"
#include "ringfifo.h"
#include "WaInit.h"
#include "http_server.h"
// #include "loto_osd.h"

#define LOTO_DBG(s32Ret)                                                       \
    do {                                                                       \
        printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__); \
    } while (0)

#define MAX_FRAME_COUNT 30

#define AUDIO_ENCODER_AAC  0xAC
#define AUDIO_ENCODER_OPUS 0xAF

typedef unsigned long ULONG;
typedef unsigned int  UINT;
typedef unsigned char BYTE;

// int     shmid;
// void*   pshm;
char     sz_pushurl[1024];
HI_U64   v_s_timestamp = 0;
HI_U64   a_s_timestamp = 0;
uint32_t start_time    = 0;

pthread_t  vid = 0, aid = 0;
static int isopen = 1;

static void* gs_rtmp     = NULL;
static char  APP_VERSION[16];
static char  gs_push_url_buf[1024] = {0};
static int   gs_audio_state        = -1;
static int   gs_audio_encoder      = -1;
static int   gs_push_algorithm     = -1;
time_t       program_start_time;
DeviceInfo   device_info = {0};

int stat_count[32] = {0};

int is_rtmp_write = 0;

HI_S32 LOTO_RTMP_VA_CLASSIC()
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

    /* Video */

    /******************************************
     step  1: init global  variable
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, PIC_HD720, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

    stVbConf.u32MaxPoolCnt = 128;

    /* video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt  = u32ViChnCnt * 15;
    memset(stVbConf.astCommPool[0].acMmzName, 0, sizeof(stVbConf.astCommPool[0].acMmzName));

    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196 * 4);
    stVbConf.astCommPool[1].u32BlkCnt  = u32ViChnCnt * 15;
    memset(stVbConf.astCommPool[1].acMmzName, 0, sizeof(stVbConf.astCommPool[1].acMmzName));

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret) {
        LOGE("system init failed with %d!\n", s32Ret);
        goto END_VENC_1HD_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret) {
        LOGE("start vi failed!\n");
        goto END_VENC_1HD_0;
    }

    /******************************************
     step 4: start vpss and vi bind vpss (subchn needn't bind vpss in this mode)
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_HD720, &stSize);
    if (HI_SUCCESS != s32Ret) {
        LOGE("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1HD_0;
    }

    stGrpAttr.u32MaxW   = stSize.u32Width;
    stGrpAttr.u32MaxH   = stSize.u32Height;
    stGrpAttr.bDrEn     = HI_FALSE;
    stGrpAttr.bDbEn     = HI_FALSE;
    stGrpAttr.bIeEn     = HI_TRUE;
    stGrpAttr.bNrEn     = HI_TRUE;
    stGrpAttr.bHistEn   = HI_FALSE; // HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stGrpAttr.enPixFmt  = SAMPLE_PIXEL_FORMAT;

    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM, &stGrpAttr);
    if (HI_SUCCESS != s32Ret) {
        LOGE("Start Vpss failed!\n");
        goto END_VENC_1HD_1;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret) {
        LOGE("Vi bind Vpss failed!\n");
        goto END_VENC_1HD_2;
    }

    enRcMode = SAMPLE_RC_VBR;

    s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad, gs_enNorm, PIC_HD720, enRcMode);
    if (HI_SUCCESS != s32Ret) {
        LOGE("Start Venc failed!\n");
        goto END_VENC_1HD_3;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VPSS_BSTR_CHN);
    if (HI_SUCCESS != s32Ret) {
        LOGE("Start Venc failed!\n");
        goto END_VENC_1HD_3;
    }

    s32Ret = LOTO_COMM_VENC_StartGetStream(u32ViChnCnt, &venc_Pid);
    if (HI_SUCCESS != s32Ret) {
        LOGE("Start Venc failed!\n");
        goto END_VENC_1HD_3;
    }

    VENC_GRP snapVencGrp = 1;
    VENC_CHN snapVencChn = 1;

    s32Ret = LOTO_COMM_VENC_CreateSnapGroup(snapVencGrp, snapVencChn, gs_enNorm, &stSize);
    if (HI_SUCCESS != s32Ret) {
        LOGE("LOTO_COMM_VENC_CreateSnapGroup failed!\n");
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(snapVencGrp, VpssGrp, VPSS_PRE0_CHN);
    if (HI_SUCCESS != s32Ret) {
        LOGE("SAMPLE_COMM_VENC_BindVpss failed!\n");
        return HI_FAILURE;
    }

    //* snap test
    // s32Ret = LOTO_COMM_VENC_GetSnapJpg();
    // if (HI_SUCCESS != s32Ret) {
    //     LOGE("LOTO_COMM_VENC_GetSnapJpg failed!\n");
    // }

    /* Audio */
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_48000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_SLAVE;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 1;
    stAioAttr.u32FrmNum      = 5;
    stAioAttr.u32PtNumPerFrm = 1024;
    stAioAttr.u32ChnCnt      = 2;
    stAioAttr.u32ClkSel      = 0;

    s32AiChnCnt   = stAioAttr.u32ChnCnt;
    s32AencChnCnt = s32AiChnCnt;

    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, HI_FALSE, NULL);
    if (s32Ret != HI_SUCCESS) {
        LOTO_DBG(s32Ret);
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, PT_LPCM);
    if (s32Ret != HI_SUCCESS) {
        LOTO_DBG(s32Ret);
        return HI_FAILURE;
    }

    for (i = 0; i < s32AencChnCnt; i++) {
        AeChn = i;
        AiChn = i;

        s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
        if (s32Ret != HI_SUCCESS) {
            LOTO_DBG(s32Ret);
            return s32Ret;
        }
    }

    s32Ret = LOTO_AUDIO_CreatTrdAenc(0, &aenc_Pid);
    if (s32Ret != HI_SUCCESS) {
        LOTO_DBG(s32Ret);
        return HI_FAILURE;
    }

    /* Add OSD */
    // s32Ret = LOTO_OSD_CreateVideoOsdThread();
    // if (s32Ret != HI_SUCCESS) {
    //     LOGE("LOTO_OSD_CreateVideoOsdThread failed! \n");
    //     return HI_FAILURE;
    // }

    pthread_join(aenc_Pid, 0);
    LOTO_AUDIO_DestoryTrdAenc(AeChn);

    for (i = 0; i < s32AencChnCnt; i++) {
        AeChn = i;
        AiChn = i;
        SAMPLE_COMM_AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
    }

    pthread_join(venc_Pid, 0);
    SAMPLE_COMM_AUDIO_StopAenc(s32AencChnCnt);
    SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, HI_FALSE, HI_FALSE);

    /******************************************
     step 8: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1HD_3:
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VPSS_BSTR_CHN);
    SAMPLE_COMM_VENC_Stop(VencGrp, VencChn);
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1HD_2: // vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_1HD_1: // vi stop
    SAMPLE_COMM_VI_Stop(enViMode);

    // pthread_create(&vid, NULL, LOTO_VENC_CLASSIC, NULL);
    // pthread_create(&aid, NULL, LOTO_AENC_AAC_CLASSIC, NULL);

END_VENC_1HD_0: // system exit
    SAMPLE_COMM_SYS_Exit();

    return HI_SUCCESS;
}

void* LOTO_VIDEO_AUDIO_RTMP(void* arg)
{
    LOGI("========== LOTO_VIDEO_AUDIO_RTMP ==========\n");

    bool a_writed = true;
    bool v_writed = true;

    uint64_t start_time     = 0;
    uint64_t a_current_time = 0;
    uint64_t v_current_time = 0;

    struct ringbuf a_ringinfo   = {0};
    int            a_ringbuflen = 0;

    struct ringbuf v_ringinfo   = {0};
    int            v_ringbuflen = 0;

    while (1) {
        if (!rtmp_sender_isOK(gs_rtmp)) {
            LOGI("Rebuild rtmp_sender\n");

            if (gs_rtmp != NULL) {
                rtmp_sender_stop_publish(gs_rtmp);
                rtmp_sender_free(gs_rtmp);
                gs_rtmp = NULL;
            }

            usleep(1000 * 100);
            gs_rtmp = rtmp_sender_alloc(gs_push_url_buf);
            if (rtmp_sender_start_publish(gs_rtmp, 0, 0) != 0) {
                LOGE("connect %s fail \n", gs_push_url_buf);
                rtmp_sender_stop_publish(gs_rtmp);
                rtmp_sender_free(gs_rtmp);
                gs_rtmp = NULL;
            }
        }

        // 判断是否可进行推流
        if (a_writed == true && gs_audio_state == HI_TRUE) {
            a_ringbuflen = ringget_audio(&a_ringinfo);
            if (a_ringbuflen > 0) {
                a_current_time = GetTimestamp(NULL, 1);

                if (start_time == 0) {
                    start_time = a_current_time;
                }

                a_writed = false;

                if (gs_rtmp != NULL) {
                    uint64_t a_timestamp = a_current_time - start_time;
                    rtmp_sender_write_aac_frame(gs_rtmp, a_ringinfo.buffer, a_ringinfo.size, a_timestamp, 0);

                    a_writed = true;
                }
            }
        }

        if (v_writed == true) {
            v_ringbuflen = ringget(&v_ringinfo);

            if (v_ringbuflen > 0) {
                v_current_time = GetTimestamp(NULL, 1);

                if (start_time == 0) {
                    start_time = v_current_time;
                }

                v_writed = false;

                if (gs_rtmp != NULL) {
                    uint64_t v_timestamp = v_current_time - start_time;
                    rtmp_sender_write_avc_frame(gs_rtmp, v_ringinfo.buffer, v_ringinfo.size, v_timestamp, 0);

                    v_writed = true;
                }
            }
        }

        a_ringbuflen = 0;
        v_ringbuflen = 0;
    }
}

void* LOTO_VIDEO_AUDIO_RTMP_1(void* p)
{
    LOGI("========== LOTO_VIDEO_AUDIO_RTMP_1 ==========\n");

    uint64_t v_timeCount    = 0;
    uint64_t a_timeCount    = 0;
    uint64_t last_timeCount = 0;

    int  v_offset  = 34;
    int  a_offset  = 22;
    bool v_writed  = true;
    bool a_writed  = true;
    int  n_v_count = 0;
    int  v_count   = 0;

    long long start_time   = 0;
    // long long a_start_time = 0;

    struct ringbuf v_ringinfo;
    int            v_ringbuflen = 0;

    struct ringbuf a_ringinfo;
    int            a_ringbuflen = 0;
    long long      t1 = 0, t2 = 0;

    while (1) {
        // if (gs_rtmp != NULL && !rtmp_sender_isOK(gs_rtmp))
        // {
        //     LOGE("[%s] rtmp is disconnected \n");
        //     rtmp_sender_stop_publish(gs_rtmp);
        //     rtmp_sender_free(gs_rtmp);
        //     sleep(1);
        //     gs_rtmp = rtmp_sender_alloc(sz_pushurl);
        //     if (rtmp_sender_start_publish(gs_rtmp, 0, 0) != 0)
        //     {
        //         LOGE("[%s] connect %s fail \n", sz_pushurl);
        //     }
        // }

        if (a_writed == true && gs_audio_state == HI_TRUE) {
            a_ringbuflen = ringget_audio(&a_ringinfo);
            // LOGD("LOTO_VIDEO_AUDIO_RTMP_1, a_ringbuflen = %d", a_ringbuflen);
            if (a_ringbuflen > 0)
                a_writed = false;
        } else {
            // LOGD("LOTO_VIDEO_AUDIO_RTMP_1, cant get audio");
        }

        if (v_writed == true) {
            v_ringbuflen = ringget(&v_ringinfo);
            // LOGD("LOTO_VIDEO_AUDIO_RTMP_1, v_ringbuflen = %d", v_ringbuflen);
            if (v_ringbuflen > 0)
                v_writed = false;
        } else {
            // LOGD("LOTO_VIDEO_AUDIO_RTMP_1, cant get video");
        }

        t1 = 0;
        t2 = 0;

        if (a_ringbuflen > 0 || v_ringbuflen > 0) {
            if (a_ringbuflen > 0)
                a_timeCount = (a_ringinfo.timestamp / 1000);

            if (v_ringbuflen > 0)
                v_timeCount = (v_ringinfo.timestamp / 1000);

            if (start_time == 0) {
                if (a_ringbuflen > 0 && v_ringbuflen > 0) {
                    if (a_timeCount <= v_timeCount) {
                        start_time = a_timeCount;
                        if (gs_rtmp != NULL)
                            rtmp_sender_write_aac_frame(gs_rtmp, a_ringinfo.buffer, a_ringinfo.size, 0, start_time);
                        a_writed       = true;
                        last_timeCount = 0;
                        a_ringbuflen   = 0;
                        // LOGD("rtmp_write_audio_0:  last_timeCount = %"PRIu64", audio timestamp = %"PRIu64"", last_timeCount,
                        // a_ringinfo.timestamp / 1000);

                        if ((a_timeCount + a_offset) > v_timeCount) {
                            is_rtmp_write = 1;
                            t1            = GetTimestampUs();
                            if (gs_rtmp != NULL)
                                rtmp_sender_write_avc_frame(gs_rtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount - start_time, 0);
                            t2            = GetTimestampUs();
                            is_rtmp_write = 0;
                            v_writed      = true;
                            n_v_count++;
                            v_count++;
                            last_timeCount = v_timeCount - start_time;
                            v_ringbuflen   = 0;
                            // LOGD("rtmp_write_video_0:  last_timeCount = %"PRIu64", video timestamp = %"PRIu64"", last_timeCount,
                            // v_ringinfo.timestamp / 1000);
                        }
                    } else {
                        start_time    = v_timeCount;
                        is_rtmp_write = 1;
                        t1            = GetTimestampUs();
                        if (gs_rtmp != NULL)
                            rtmp_sender_write_avc_frame(gs_rtmp, v_ringinfo.buffer, v_ringinfo.size, 0, 0);

                        t2            = GetTimestampUs();
                        is_rtmp_write = 0;
                        v_writed      = true;
                        n_v_count++;
                        v_count++;
                        last_timeCount = 0;
                        v_ringbuflen   = 0;
                        // LOGD("rtmp_write_video_0_1:  last_timeCount = %"PRIu64", video timestamp = %"PRIu64"", last_timeCount,
                        // v_ringinfo.timestamp / 1000);

                        if ((v_timeCount + v_offset) > a_timeCount) {
                            if (gs_rtmp != NULL)
                                rtmp_sender_write_aac_frame(gs_rtmp, a_ringinfo.buffer, a_ringinfo.size, a_timeCount - start_time, start_time);

                            a_writed       = true;
                            last_timeCount = a_timeCount - start_time;
                            a_ringbuflen   = 0;
                            // LOGD("rtmp_write_audio_0_1:  last_timeCount = %"PRIu64", audio timestamp = %"PRIu64"", last_timeCount,
                            // a_ringinfo.timestamp / 1000);
                        }
                    }

                } else if (a_ringbuflen > 0) {
                    start_time = a_timeCount;
                    if (gs_rtmp != NULL)
                        rtmp_sender_write_aac_frame(gs_rtmp, a_ringinfo.buffer, a_ringinfo.size, 0, start_time);

                    a_writed       = true;
                    last_timeCount = 0;
                    a_ringbuflen   = 0;
                    // LOGD("rtmp_write_audio_0_2:  last_timeCount = %"PRIu64", audio timestamp = %"PRIu64"", last_timeCount,
                    // a_ringinfo.timestamp / 1000);

                } else if (v_ringbuflen > 0) {
                    start_time    = v_timeCount;
                    is_rtmp_write = 1;
                    t1            = GetTimestampUs();
                    if (gs_rtmp != NULL)
                        rtmp_sender_write_avc_frame(gs_rtmp, v_ringinfo.buffer, v_ringinfo.size, 0, 0);
                    t2            = GetTimestampUs();
                    is_rtmp_write = 0;
                    v_writed      = true;
                    n_v_count++;
                    v_count++;
                    last_timeCount = 0;
                    v_ringbuflen   = 0;
                    // LOGD("rtmp_write_video_0_2:  last_timeCount = %"PRIu64", video timestamp = %"PRIu64"", last_timeCount,
                    // v_ringinfo.timestamp / 1000);
                }

            } else {
                if (a_ringbuflen > 0 && v_ringbuflen > 0) {
                    if (a_timeCount <= v_timeCount) {
                        if (gs_rtmp != NULL)
                            rtmp_sender_write_aac_frame(gs_rtmp, a_ringinfo.buffer, a_ringinfo.size, a_timeCount - start_time, start_time);
                        a_writed       = true;
                        last_timeCount = a_timeCount - start_time;
                        a_ringbuflen   = 0;
                        // LOGD("rtmp_write_audio_1:  last_timeCount = %"PRIu64", audio timestamp = %"PRIu64"", last_timeCount,
                        // a_ringinfo.timestamp / 1000);

                        if ((a_timeCount + a_offset) > v_timeCount) {
                            is_rtmp_write = 1;
                            t1            = GetTimestampUs();
                            if (gs_rtmp != NULL)
                                rtmp_sender_write_avc_frame(gs_rtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount - start_time, 0);
                            t2             = GetTimestampUs();
                            is_rtmp_write  = 0;
                            v_writed       = true;
                            last_timeCount = v_timeCount - start_time;
                            v_ringbuflen   = 0;

                            if (n_v_count == 5) {
                                v_offset  = 34;
                                n_v_count = 0;
                            } else
                                v_offset = 17;
                            n_v_count++;
                        }

                    } else {
                        t1 = GetTimestampUs();
                        if (v_timeCount < start_time)
                            v_timeCount = start_time;
                        is_rtmp_write = 1;
                        if (gs_rtmp != NULL)
                            rtmp_sender_write_avc_frame(gs_rtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount - start_time, 0);
                        t2            = GetTimestampUs();
                        is_rtmp_write = 0;

                        v_writed       = true;
                        last_timeCount = v_timeCount - start_time;
                        v_ringbuflen   = 0;

                        if (n_v_count == 5) {
                            v_offset  = 34;
                            n_v_count = 0;
                        } else
                            v_offset = 17;
                        n_v_count++;

                        if ((v_timeCount + v_offset) > a_timeCount) {
                            if (gs_rtmp != NULL)
                                rtmp_sender_write_aac_frame(gs_rtmp, a_ringinfo.buffer, a_ringinfo.size, a_timeCount - start_time, start_time);

                            a_writed       = true;
                            last_timeCount = a_timeCount - start_time;
                            a_ringbuflen   = 0;
                            // LOGD("rtmp_write_audio_1_1:  last_timeCount = %"PRIu64"", last_timeCount);
                        }
                    }

                } else if (a_ringbuflen > 0) {
                    if ((last_timeCount + v_offset) >= a_timeCount) {
                        if (gs_rtmp != NULL)
                            rtmp_sender_write_aac_frame(gs_rtmp, a_ringinfo.buffer, a_ringinfo.size, a_timeCount - start_time, start_time);

                        a_writed       = true;
                        last_timeCount = a_timeCount - start_time;
                        a_ringbuflen   = 0;
                        // LOGD("rtmp_write_audio_1_2:  last_timeCount = %"PRIu64"", last_timeCount);
                    }
                } else if (v_ringbuflen > 0) {
                    if (gs_audio_state == HI_FALSE || ((last_timeCount + a_offset) >= v_timeCount)) {
                        is_rtmp_write = 1;
                        t1            = GetTimestampUs();
                        if (gs_rtmp != NULL)
                            rtmp_sender_write_avc_frame(gs_rtmp, v_ringinfo.buffer, v_ringinfo.size, v_timeCount - start_time, 0);

                        t2             = GetTimestampUs();
                        is_rtmp_write  = 0;
                        v_writed       = true;
                        last_timeCount = v_timeCount - start_time;
                        v_ringbuflen   = 0;

                        if (n_v_count == 5) {
                            v_offset  = 34;
                            n_v_count = 0;

                        } else {
                            v_offset = 17;
                        }

                        n_v_count++;
                    }
                }
            }

            if (t1 != 0 && t2 != 0) {
                int offset = (int)((t2 - t1) / 1000);

                if (offset == 0) {
                    stat_count[0]++;

                } else if (offset == 1) {
                    stat_count[1]++;

                } else if (offset >= 5 * 10) {
                    stat_count[12]++;

                } else {
                    stat_count[offset / 5 + 2]++;
                }

                if (v_count == 0 || v_count == 3000) {
                    LOGD("rtmp_write_video:  last_timeCount = %" PRIu64 ", video timestamp = %" PRIu64 "", last_timeCount,
                         v_ringinfo.timestamp / 1000);

                    int  i            = 0;
                    char s_print[512] = "";
                    sprintf(s_print, "%d", stat_count[0]);
                    stat_count[0] = 0;
                    for (i = 1; i < 13; i++) {
                        sprintf(s_print, "%s, %d", s_print, stat_count[i]);
                        stat_count[i] = 0;
                    }
                    LOGD("rtmp_write_video: statistics write time = %s", s_print);

                    v_count = 0;
                }
                v_count++;
            }

        } else {
            a_writed = true;
            v_writed = true;
        }
        usleep(500);
    }
}

void parse_config_file(const char* config_file_path)
{
    /* Get default push_url */
    strcpy(gs_push_url_buf, GetConfigKeyValue("push", "push_url", config_file_path));

    /* If push address should be requested, server address must be set first */
    if (strncmp("on", GetConfigKeyValue("push", "requested_url", config_file_path), 2) == 0) {
        /* Get server token */
        char server_token[1024] = {0};
        strcpy(server_token, GetConfigKeyValue("push", "server_token", config_file_path));
        // LOGD("server_token = %s\n", server_token);

        /* Get server url */
        char server_url[1024];

        strcpy(server_url, GetConfigKeyValue("push", "server_url", config_file_path));
        strcpy(device_info.server_url, server_url);

        LOGI("server_url = %s\n", server_url);

        loto_room_info* pRoomInfo = loto_room_init(server_url, server_token);
        if (!pRoomInfo) {
            LOGE("room_info is empty\n");
            exit(2);
        }
        memset(gs_push_url_buf, 0, sizeof(gs_push_url_buf));
        strcpy(gs_push_url_buf, pRoomInfo->szPushURL);
    }
    strcpy(device_info.push_url, gs_push_url_buf);
    LOGI("push_url = %s\n", gs_push_url_buf);

    /* device num */
    // strncpy(g_device_num, GetConfigKeyValue("device", "device_num",
    // config_file_path), 3); strcpy(device_info.device_num, g_device_num);
    // LOGI("device_num = %s\n", g_device_num);

    const char* video_encoder = GetConfigKeyValue("push", "video_encoder", config_file_path);
    strcpy(device_info.video_encoder, video_encoder);
    LOGI("video_encoder = %s\n", video_encoder);

    /* audio_state */
    char* audio_state = GetConfigKeyValue("push", "audio_state", config_file_path);
    strcpy(device_info.audio_state, audio_state);
    if (strncmp("off", audio_state, 3) == 0) {
        gs_audio_state = HI_FALSE;
    } else if (strncmp("on", audio_state, 2) == 0) {
        gs_audio_state = HI_TRUE;

        /* audio_encoder */
        const char* audio_encoder = GetConfigKeyValue("push", "audio_encoder", config_file_path);
        strcpy(device_info.audio_encoder, audio_encoder);
        LOGI("audio_encoder = %s\n", audio_encoder);

        if (strncmp("aac", audio_encoder, 3) == 0) {
            gs_audio_encoder = AUDIO_ENCODER_AAC;

        } else if (strncmp("opus", audio_encoder, 4) == 0) {
            gs_audio_encoder = AUDIO_ENCODER_OPUS;

        } else {
            LOGE("The set value of audio_encoder is %s! The supported audio "
                 "encoders: aac, opus\n",
                 audio_encoder);
            exit(1);
        }
    } else {
        LOGE("The set value of audio_state is %s! Please set on or off.\n", audio_state);
        exit(1);
    }

    gs_push_algorithm = GetIniKeyInt("push", "push_algo", config_file_path);
}

void fill_device_net_info(DeviceInfo* device_info)
{
    get_local_ip_address(device_info->ip_addr);
    get_local_mac_address(device_info->mac_addr);
}

#define VER_MAJOR 0
#define VER_MINOR 3
#define VER_BUILD 5

int main(int argc, char* argv[])
{
    int        s32Ret             = 0;
    const char config_file_path[] = PUSH_CONFIG_FILE_PATH;

    /* Initialize avctl_log file */
    s32Ret = InitAvctlLogFile();
    if (s32Ret != 0) {
        LOGE("Create avctl_log file failed! \n");
        return -1;
    }

    /* Initialize rtmp_log file */
    FILE* rtmp_log;
    s32Ret = InitRtmpLogFile(&rtmp_log);
    if (s32Ret != 0) {
        LOGE("Create rtmp_log file failed! \n");
    } else {
        RTMP_LogSetOutput(rtmp_log);
    }

    /* sync local time from net_time */
    s32Ret = get_net_time();
    if (s32Ret != HI_SUCCESS) {
        LOGE("Time sync failed\n");
        exit(1);
    }

    /* Gets the program startup time */
    program_start_time = time(NULL);
    strcpy(device_info.start_time, GetTimestampString());

    sprintf(APP_VERSION, "%d.%d.%d", VER_MAJOR, VER_MINOR, VER_BUILD);
    strcpy(device_info.app_version, APP_VERSION);
    LOGI("402 ENCODER: RTMP App Version: %s\n", APP_VERSION);

    /* get global variables from config file */
    parse_config_file(config_file_path);
    fill_device_net_info(&device_info);

    ringmalloc(500 * 1024);
    ringmalloc_audio(1024 * 5);

    gs_rtmp = rtmp_sender_alloc(gs_push_url_buf);
    if (rtmp_sender_start_publish(gs_rtmp, 0, 0) != 0) {
        LOGD("connect %s fail\n", gs_push_url_buf);
        rtmp_sender_free(gs_rtmp);
        gs_rtmp = NULL;
        return -1;
    }

    pthread_t rtmp_pid;
    if (gs_push_algorithm == 0) {
        pthread_create(&rtmp_pid, NULL, LOTO_VIDEO_AUDIO_RTMP, NULL);

    } else if (gs_push_algorithm == 1) {
        pthread_create(&rtmp_pid, NULL, LOTO_VIDEO_AUDIO_RTMP_1, NULL);
    }
    usleep(1000 * 10);

    pthread_t sync_time_pid;
    if (pthread_create(&sync_time_pid, NULL, sync_time, NULL) != 0) {
        fprintf(stderr, "Failed to create sync_time thread\n");
    }
    usleep(100);

    pthread_t http_server_thread;
    if (pthread_create(&http_server_thread, NULL, http_server, NULL) != 0) {
        fprintf(stderr, "Failed to create http_server thread\n");
    }

    LOTO_RTMP_VA_CLASSIC();

    pthread_join(rtmp_pid, 0);
    

    // while (1) {
    //     sleep(1);
    // }
    // printf("end\n");

    LOGI("=================== END =======================\n");

    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */