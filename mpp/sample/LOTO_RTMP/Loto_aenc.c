/******************************************************************************

  Copyright (C), 2022, Lotogram Tech. Co., Ltd.

 ******************************************************************************
  File Name     : Loto_venc.c
  Version       : Initial Draft
  Author        : 
  Created       : 2022
  Description   :
******************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <inttypes.h>
#include "sample_comm.h"
#include "common.h"
#include "ringfifo.h"
#include "EasyAACEncoderAPI.h"
#include "faac.h"
#include "fdk-aac/aacenc_lib.h"


typedef struct tagLOTO_AENC_S
{
    HI_BOOL bStart;
    pthread_t stAencPid;
    HI_S32  AeChn;
} LOTO_AENC_S;

typedef struct tagLOTO_AI_S
{
    HI_BOOL bStart;
    pthread_t stAiPid;
    HI_S32  AiChn;
} LOTO_AI_S;

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4/* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_40K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */

static LOTO_AENC_S gs_stLotoAenc[AENC_MAX_CHN_NUM];
static LOTO_AI_S gs_stLotoAi[AIO_MAX_CHN_NUM];

#define SAMPLE_AUDIO_PTNUMPERFRM   1024
#define SAMPLE_AUDIO_AI_DEV 0

static PAYLOAD_TYPE_E gs_enPayloadType = PT_LPCM;
// static Easy_Handle g_Easy_H = NULL;

#define AAC_ADTS_HEADER_SIZE 7

// typedef unsigned long ULONG;
// typedef unsigned int UINT;
// typedef unsigned char BYTE;

extern Easy_Handle g_Easy_H;
// extern faacEncHandle hEncoder;
// extern ULONG nMaxOutputBytes;


#define LOTO_DBG(s32Ret)\
    do{\
        printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
    }while(0)


#include <unistd.h>
#include <time.h>
static unsigned long long os_get_reltime(void)
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (tp.tv_sec * 1000000ULL + tp.tv_nsec / 1000ULL);
}


uint8_t *get_adts(uint32_t *len, uint8_t **offset, uint8_t *start, uint32_t total)
{
    uint8_t *p  =  *offset;
    uint32_t frame_len_1;
    uint32_t frame_len_2;
    uint32_t frame_len_3;
    uint32_t frame_length;
   
    if (total < AAC_ADTS_HEADER_SIZE) {
        return NULL;
    }
    if ((p - start) >= total) {
        return NULL;
    }
    
    if (p[0] != 0xff) {
        return NULL;
    }
    if ((p[1] & 0xf0) != 0xf0) {
        return NULL;
    }
    frame_len_1 = p[3] & 0x03;
    frame_len_2 = p[4];
    frame_len_3 = (p[5] & 0xe0) >> 5;
    frame_length = (frame_len_1 << 11) | (frame_len_2 << 3) | frame_len_3;
    *offset = p + frame_length;
    *len = frame_length;
    return p;
}


/******************************************************************************
* function : get stream from Aenc, send it  to ringfifo
******************************************************************************/
void* LOTO_COMM_AUDIO_AencProc(void* parg)
{
    HI_S32 s32Ret;
    HI_S32 AencFd[2];
    AENC_CHN AencChn;
    HI_S32 maxfd = 0;
    int i = 0, k = 0;
    FILE *pfd[2];

    LOTO_AENC_S* pstAencCtl = (LOTO_AENC_S*)parg;
    AUDIO_STREAM_S stStream;
    fd_set read_fds;
    struct timeval TimeoutVal;
    HI_U64 u64TimeStamp = 0, lastTimeStamp = 0;
    int iHead = 1;
    // HI_U8* pStream = NULL;
    HI_U32 u32Len = 0;
    unsigned int out_len = 0;
    uint8_t *audio_buf_offset ;
    uint32_t audio_len;
    uint8_t *p_audio;

    HANDLE_AACENCODER   g_Enc_H = NULL;
    AACENC_PARAM        aacenc_param;

    if (aacEncOpen(&g_Enc_H, 0, 2) != AACENC_OK)
    {
        printf("[ERROR] Failed to call aacEncOpen()\n");
        return  NULL;
    }
    if (aacEncoder_SetParam(g_Enc_H, AACENC_AOT, 2)!= AACENC_OK)
    {
        aacEncClose(g_Enc_H);
        printf("[ERROR] Unable to set the AOT\n");
        return  NULL;
    }
    // aacEncoder_SetParam(g_Enc_H, AACENC_BITRATEMODE, 4);
    if (aacEncoder_SetParam(g_Enc_H, AACENC_BITRATE, 128000) != AACENC_OK)
    {
        aacEncClose(g_Enc_H);
        printf("[ERROR] Unable to set the Bitrate\n");
        return  NULL;
    }

    // if (aacEncoder_SetParam(g_Enc_H, AACENC_CHANNELORDER, 1) != AACENC_OK)
    // {
    //     aacEncClose(g_Enc_H);
    //     printf("[ERROR] Unable to set the ChannelOrder\n");
    //     return  NULL;
    // }
    if (aacEncoder_SetParam(g_Enc_H, AACENC_SAMPLERATE, 48000) != AACENC_OK)
    {
        aacEncClose(g_Enc_H);
        printf("[ERROR] Unable to set the SampleRate\n");
        return  NULL;
    }
    if (aacEncoder_SetParam(g_Enc_H, AACENC_GRANULE_LENGTH, 1024) != AACENC_OK)
    {
        aacEncClose(g_Enc_H);
        printf("[ERROR] Unable to set the Granule length\n");
        return  NULL;
    }
    if (aacEncoder_SetParam(g_Enc_H, AACENC_CHANNELMODE, MODE_2) != AACENC_OK)
    {
        aacEncClose(g_Enc_H);
        printf("[ERROR] Unable to set the ChannelMode\n");
        return  NULL;
    }
    if (aacEncoder_SetParam(g_Enc_H, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK)
    {
        aacEncClose(g_Enc_H);
        printf("[ERROR] Unable to set the TransMux\n");
        return  NULL;
    }

    if (aacEncEncode(g_Enc_H, NULL, NULL, NULL, NULL) != AACENC_OK)
    {
        aacEncClose(g_Enc_H);
        printf("[ERROR] Unable to initialize the encoder\n");
        return  NULL;
    }
                    
    // int bAACBufferSize = 5096;//提供足够大的缓冲区
    // unsigned char *pbAACBuffer = (unsigned char*)malloc(bAACBufferSize * sizeof(unsigned char)); 
    // pStream = (HI_U8*)malloc(bAACBufferSize * sizeof(HI_U8)); 

    // ULONG nSampleRate = 44100;  // 采样率
    // UINT nChannels = 1;         // 声道数
    // UINT nBit = 16;             // 单样本位数
    // ULONG nInputSamples = 0;	//输入样本数
	// ULONG nMaxInputBytes=0;     //输入最大字节
    // faacEncHandle hEncoder;		//aac句柄
    // faacEncConfigurationPtr pConfiguration;//aac设置指针
    // ULONG nMaxOutputBytes = 0;	//输出所需最大空间

    // hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);//初始化aac句柄，同时获取最大输入样本，及编码所需最小字节
	// nMaxInputBytes=nInputSamples*nBit/8;//计算最大输入字节,跟据最大输入样本数
	// printf("nInputSamples:%d nMaxInputBytes:%d nMaxOutputBytes:%d\n", nInputSamples, nMaxInputBytes,nMaxOutputBytes);
    
    // if(hEncoder == NULL)
    // {
    //     printf("[ERROR] Failed to call faacEncOpen()\n");
    //     return -1;
    // }
    // // pbAACBuffer = (char*)malloc(nMaxOutputBytes);
    // // (2.1) Get current encoding configuration
    // pConfiguration = faacEncGetCurrentConfiguration(hEncoder);//获取配置结构指针
    // pConfiguration->inputFormat = FAAC_INPUT_16BIT;
	// pConfiguration->outputFormat=1;
	// pConfiguration->useTns=0;
	// // pConfiguration->useLfe=0;
	// pConfiguration->aacObjectType=LOW;
	// pConfiguration->mpegVersion = MPEG4; 
	// // pConfiguration->shortctl=SHORTCTL_NORMAL;
	// // pConfiguration->quantqual=100;
	// pConfiguration->bandWidth=0;
	// // pConfiguration->bitRate=44100;
    // pConfiguration->allowMidside = 1;
	// printf("!!!!!!!!!!!!!pConfiguration->outputFormat is %d\n",pConfiguration->outputFormat);
    // // (2.2) Set encoding configuration
    // faacEncSetConfiguration(hEncoder, pConfiguration);//设置配置，根据不同设置，耗时不一样


    // unsigned char streamBuf[1024*5];
    // unsigned char aacBuf[2048];

    char szServerFile[256] = "";
	sprintf(szServerFile, "%s/server.ini", WORK_FOLDER);

    int i_writefile = GetIniKeyInt((char*)"server", (char*)"writefile", szServerFile);

    // BYTE* pbAACBuffer = (char*)malloc(nMaxOutputBytes);

    // FD_ZERO(&read_fds);
    // AencFd = HI_MPI_AENC_GetFd(pstAencCtl->AeChn);

    // FD_SET(AencFd, &read_fds);

    for (i = 0; i < 2; i++)
    {
       /* Set Venc Fd. */
        AencFd[i] = HI_MPI_AENC_GetFd(i);
        if (AencFd[i] < 0)
        {
            SAMPLE_PRT("HI_MPI_AENC_GetFd failed with %#x!\n", 
                   AencFd[i]);
            return NULL;
        }
        if (maxfd <= AencFd[i])
        {
            maxfd = AencFd[i];
        }

        if (i_writefile == 1)
        {
            HI_CHAR aszFileName[128];
    
            /* create file for save stream*/
            sprintf(aszFileName, "audio_chn%d.aac", i);
            pfd[i] = fopen(aszFileName, "w+");
        }
    }

    long long ll_us_sys_timestamp = 0;
    long long ll_first_timestamp = 0;

    int  n_count = 0;
    while (pstAencCtl->bStart)
    {
        // FD_ZERO(&read_fds);
        // FD_SET(AencFd,&read_fds); 

        FD_ZERO(&read_fds);
        for (i = 0; i < 2; i++)
        {
            FD_SET(AencFd[i], &read_fds);
        }
    
        TimeoutVal.tv_sec = 0;
        TimeoutVal.tv_usec = 0;

        s32Ret = select(maxfd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            break;
        }
        else if (0 == s32Ret)
        {
            // printf("%s: get aenc stream select time out\n", __FUNCTION__);
            usleep(500);
            continue;
        }

        u32Len = 0;
        // memset(aacBuf, 0, sizeof(aacBuf));
        // memset(streamBuf, 0, 1024*5);

        for (i = 0; i < 2; i ++)
        {
            if (FD_ISSET(AencFd[i], &read_fds))
            {
                /* get stream from aenc chn */
                s32Ret = HI_MPI_AENC_GetStream(i, &stStream, HI_FALSE);
                if (HI_SUCCESS != s32Ret )
                {
                    printf("%s: HI_MPI_AENC_GetStream(%d), failed with %#x!\n",\
                        __FUNCTION__, i, s32Ret);
                    pstAencCtl->bStart = HI_FALSE;
                    return NULL;
                }

                if (n_count == 0 || n_count == 3000)
                {
                    // LOGD("[%s] LOTO_COMM_AUDIO_AencProc:  timestamp = %"PRIu64"", log_Time(), stStream.u64TimeStamp/1000);
                    n_count = 0;
                }
                n_count ++;

                if (i == 0)
                {
                    // memcpy(streamBuf+u32Len, stStream.pStream, stStream.u32Len);
                    u32Len += stStream.u32Len;
                    lastTimeStamp = stStream.u64TimeStamp;

                    AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
                    AACENC_InArgs in_args = { 0 };
                    AACENC_OutArgs out_args = { 0 };
                    int in_identifier = IN_AUDIO_DATA;
                    int in_size, in_elem_size;
                    int out_identifier = OUT_BITSTREAM_DATA;
                    int out_size, out_elem_size;
                    int read, i;
                    void *in_ptr, *out_ptr;
                    unsigned char outbuf[2048];
                    uint16_t inbuf[2048];
                    AACENC_ERROR err;

                    for (k = 0; k < stStream.u32Len; k += 2)
                    {
                        // inbuf[2*k] = stStream.pStream[k];
                        // inbuf[2*k+1] = stStream.pStream[k+1];
                        // inbuf[2*k+2] = stStream.pStream[k];
                        // inbuf[2*k+3] = stStream.pStream[k+1];

                        inbuf[k+1] = inbuf[k] = stStream.pStream[k] | (stStream.pStream[k+1] << 8);
                    }

                    in_size = stStream.u32Len*2;
                    in_ptr = inbuf;
                    in_elem_size = 2;

                    in_args.numInSamples = stStream.u32Len/2;
                    in_buf.numBufs = 1;
                    in_buf.bufs = &in_ptr;
                    in_buf.bufferIdentifiers = &in_identifier;
                    in_buf.bufSizes = &in_size;
                    in_buf.bufElSizes = &in_elem_size;

                    out_ptr = outbuf;
                    out_size = sizeof(outbuf);
                    out_elem_size = 1;
                    out_buf.numBufs = 1;
                    out_buf.bufs = &out_ptr;
                    out_buf.bufferIdentifiers = &out_identifier;
                    out_buf.bufSizes = &out_size;
                    out_buf.bufElSizes = &out_elem_size;

                    if(aacEncEncode(g_Enc_H, &in_buf, &out_buf, &in_args, &out_args) == AACENC_OK)
                    {
                        if(out_args.numOutBytes > 0)
                        {
                            HisiPutAACDataToBuffer(outbuf, out_args.numOutBytes, lastTimeStamp, 0);
                            // fwrite(outbuf, 1, out_args.numOutBytes, pfd);
                            // LOGD("[%s] %s: i = %d, aenc stream length: %d, aac length = %d \n", log_Time(), __FUNCTION__, i, stStream.u32Len, out_args.numOutBytes);
                        }
                        else
                        {
                            printf("%s: Encoding 0 bytes\n", __FUNCTION__);
                        }
                    }
                    else
                    {
                        printf("%s: Encoding failed\n", __FUNCTION__);
                    }
                }

                // if (ll_us_sys_timestamp == 0)
                // {
                //     ll_us_sys_timestamp = get_us_timestamp();
                //     ll_first_timestamp = stStream.u64TimeStamp;
                // }

                // stStream.u64TimeStamp = ll_us_sys_timestamp+(stStream.u64TimeStamp-ll_first_timestamp);
                
                    // LOGD("[%s] HI_MPI_AENC_GetStream: i = %d, timestamp = %"PRIu64", stream Len = %ld \n", log_Time(), i, lastTimeStamp/1000, stStream.u32Len);

                    // int ret = faacEncEncode(hEncoder, (int*) stStream.pStream, u32Len/2, pbAACBuffer, nMaxOutputBytes);
                    // audio_buf_offset = pbAACBuffer;
                    // if (ret > 0)
                    // {
                    //     // LOGD("[%s] faacEncEncode \n", log_Time());
                    //     p_audio = get_adts(&audio_len, &audio_buf_offset, pbAACBuffer, ret);
                    //     if (p_audio != NULL){
                    //         HisiPutAACDataToBuffer(p_audio, audio_len, lastTimeStamp, 0);
                    //         // if (ret != audio_len)
                    //             // LOGD("[%s] LOTO_COMM_AUDIO_AencProc:  Encoder Out_Len = %ld, Audio Length = %ld", log_Time(), ret, audio_len);
                    //     }
                    //     else
                    //     {
                    //         LOGD("[%s] LOTO_COMM_AUDIO_AencProc:  Audio Encode invalid", log_Time());
                    //     }
                    // }

                // LOGD("[%s] LOTO_COMM_AUDIO_AencProc:  Encoder Out_Len1 = %ld, Out_Len2 = %ld, Audio Length = %ld", log_Time(), out_len, ret, u32Len);	


                HI_MPI_AENC_ReleaseStream(i, &stStream);
                
                /* finally you must release the stream */
                
            }
        }

        // if (u32Len > 0)
        // {
        //     int ret = faacEncEncode(hEncoder, (int*) streamBuf, u32Len/2, pbAACBuffer, nMaxOutputBytes);
        //     audio_buf_offset = pbAACBuffer;
        //     if (ret > 0)
        //     {
        //         // LOGD("[%s] faacEncEncode \n", log_Time());
        //         p_audio = get_adts(&audio_len, &audio_buf_offset, pbAACBuffer, ret);
        //         if (p_audio != NULL){
        //             HisiPutAACDataToBuffer(p_audio, audio_len, lastTimeStamp, 0);
        //             // if (ret != audio_len)
        //                 // LOGD("[%s] LOTO_COMM_AUDIO_AencProc:  Encoder Out_Len = %ld, Audio Length = %ld", log_Time(), ret, audio_len);
        //         }
        //         else
        //         {
        //             LOGD("[%s] LOTO_COMM_AUDIO_AencProc:  Audio Encode invalid", log_Time());
        //         }
        //     }
        // }

        // if (u32Len > 0)
        // {
        //     if((Easy_AACEncoder_Encode(g_Easy_H, streamBuf, u32Len, aacBuf, &out_len)) > 0)
        //     {
        //         HisiPutAACDataToBuffer(aacBuf, out_len, lastTimeStamp, 0);
        //         LOGD("[%s] LOTO_COMM_AUDIO_AencProc:  Encoder Out_Len = %ld, PCM Len = %ld, timestamp = %"PRIu64"", log_Time(), out_len, u32Len, lastTimeStamp/1000);

        //         if (i_writefile == 1 && pfd[0] != NULL)
        //             fwrite(aacBuf, 1, out_len, pfd[0]);

        //         // LOGD("[%s] Easy_AACEncoder_Encode \n", log_Time());
        //         // audio_buf_offset = aacBuf;
        //         // p_audio = get_adts(&audio_len, &audio_buf_offset, aacBuf, out_len);
        //         // if (p_audio != NULL){
        //         //     HisiPutAACDataToBuffer(p_audio, audio_len, lastTimeStamp, 0);
        //         //     if (out_len != audio_len)
        //         //         LOGD("[%s] LOTO_COMM_AUDIO_AencProc:  Encoder Out_Len = %ld, Audio Length = %ld", log_Time(), out_len, audio_len);
        //         // }
        //         // else
        //         // {
        //         //     LOGD("[%s] LOTO_COMM_AUDIO_AencProc:  Audio Encode invalid", log_Time());
        //         // }
        //     }
        // }
    }

    // free(pbAACBuffer);
    // free(pStream);
    // if (pbAACBuffer != NULL)
    //     free(pbAACBuffer);

    for (i = 0; i < 1; i ++)
    {
        if (pfd[i] != NULL)
            fclose(pfd[i]);
    }
    // faacEncClose(hEncoder);

    aacEncClose(g_Enc_H);

    pstAencCtl->bStart = HI_FALSE;
    return NULL;
}

/******************************************************************************
* function : Destory the thread to get stream from aenc
******************************************************************************/
HI_S32 LOTO_AUDIO_DestoryTrdAenc(AENC_CHN AeChn)
{
    LOTO_AENC_S* pstAenc = NULL;

    pstAenc = &gs_stLotoAenc[AeChn];
    if (pstAenc->bStart)
    {
        pstAenc->bStart = HI_FALSE;
    }


    return HI_SUCCESS;
}

/******************************************************************************
* function : Create the thread to get stream from aenc and send to ringfifo
******************************************************************************/
HI_S32 LOTO_AUDIO_CreatTrdAenc(AENC_CHN AeChn, pthread_t* aencPid)
{
    LOTO_AENC_S* pstAenc = NULL;

    pstAenc = &gs_stLotoAenc[AeChn];
    pstAenc->AeChn = AeChn;
    pstAenc->bStart = HI_TRUE;
    HI_S32 nRet = pthread_create(&pstAenc->stAencPid, 0, LOTO_COMM_AUDIO_AencProc, pstAenc);

    if (nRet == 0)
    {
        if (aencPid != NULL)
            *aencPid = pstAenc->stAencPid;
        else
            pthread_join(pstAenc->stAencPid, 0);
    }
    return nRet;
}

void* LOTO_COMM_AUDIO_AiProc(void* parg)
{
    HI_S32 s32Ret;
    HI_S32 AiFd;
    LOTO_AI_S* pstAiCtl = (LOTO_AI_S*)parg;
    AUDIO_STREAM_S stStream;
    AUDIO_FRAME_S audio_Frame;
    fd_set read_fds;
    struct timeval TimeoutVal;

    FD_ZERO(&read_fds);
    AiFd = HI_MPI_AI_GetFd(SAMPLE_AUDIO_AI_DEV, pstAiCtl->AiChn);
    FD_SET(AiFd, &read_fds);

    while (pstAiCtl->bStart)
    {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(AiFd,&read_fds);

        s32Ret = select(AiFd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            break;
        }
        else if (0 == s32Ret)
        {
            printf("%s: get ai stream select time out\n", __FUNCTION__);
            break;
        }

        if (FD_ISSET(AiFd, &read_fds))
        {
            /* get frame from ai chn */
            s32Ret = HI_MPI_AI_GetFrame(SAMPLE_AUDIO_AI_DEV, pstAiCtl->AiChn, &audio_Frame, NULL, HI_FALSE);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AI_GetFrame(%d), failed with %#x!\n",\
                       __FUNCTION__, pstAiCtl->AiChn, s32Ret);
                pstAiCtl->bStart = HI_FALSE;
                return NULL;
            }

            int bAACBufferSize = audio_Frame.u32Len;//提供足够大的缓冲区
            unsigned char *pbAACBuffer = (unsigned char*)malloc(bAACBufferSize * sizeof(unsigned char));  
            unsigned int out_len = 0;
            unsigned long long ts = os_get_reltime();
            long long cur_ts = get_timestamp(NULL, 1);

            if((Easy_AACEncoder_Encode(g_Easy_H, audio_Frame.pVirAddr[0], audio_Frame.u32Len, pbAACBuffer, &out_len)) > 0)
            {
                // HisiPutAACDataToBuffer(pbAACBuffer, out_len, stStream.u64TimeStamp);
                unsigned long long ts1 = os_get_reltime();
                long long cur_ts1 = get_timestamp(NULL, 1);

                printf("stream Len = %d, aac Len = %d \n", audio_Frame.u32Len, out_len);

                // printf("stream Time = %"PRIu64", before encoder = %"PRIu64", after encoder = %"PRIu64" \n", stStream.u64TimeStamp, ts, ts1);
                // printf("before encoder = %lld, after encoder = %lld \n", cur_ts, cur_ts1);

                // printf("%s:[%d] pbAACBuffer(%d) len=%d, ts = %"PRIu64", ts1 = %"PRIu64" \n",__FUNCTION__,__LINE__, bAACBufferSize, out_len, ts, ts1);
            }
            free(pbAACBuffer);

            //HisiPutAACDataToBuffer(&stStream);
            /* finally you must release the stream */
            HI_MPI_AI_ReleaseFrame(SAMPLE_AUDIO_AI_DEV, pstAiCtl->AiChn, &audio_Frame, NULL);
        }
    }

    pstAiCtl->bStart = HI_FALSE;
    return NULL;
}


/******************************************************************************
* function : Create the thread to get stream from ai and send to ringfifo
******************************************************************************/
HI_S32 LOTO_AUDIO_CreatTrdAi(AI_CHN AiChn, pthread_t* aiPid)
{
    LOTO_AI_S* pstAi = NULL;

    pstAi = &gs_stLotoAi[AiChn];
    pstAi->AiChn = AiChn;
    pstAi->bStart = HI_TRUE;
    HI_S32 nRet = pthread_create(&pstAi->stAiPid, 0, LOTO_COMM_AUDIO_AiProc, pstAi);

    if (nRet == 0)
    {
        if (aiPid != NULL)
            *aiPid = pstAi->stAiPid;
        else
            pthread_join(pstAi->stAiPid, 0);
    }
    return nRet;
}

/******************************************************************************
* function : Destory the thread to get stream from aenc
******************************************************************************/
HI_S32 LOTO_AUDIO_DestoryTrdAi(AI_CHN AiChn)
{
    LOTO_AI_S* pstAi = NULL;

    pstAi = &gs_stLotoAi[AiChn];
    if (pstAi->bStart)
    {
        pstAi->bStart = HI_FALSE;
    }


    return HI_SUCCESS;
}

/******************************************************************************
* function : Ai -> Aenc -> buffer
******************************************************************************/
HI_S32 LOTO_AUDIO_AiAenc(HI_VOID)
{
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    HI_S32 i, s32Ret= HI_SUCCESS;
    AIO_ATTR_S stAioAttr;
    HI_S32      s32AiChnCnt;
    HI_S32      s32AencChnCnt;
    AENC_CHN    AeChn;
    AI_CHN      AiChn;

    stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_48000;
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt = 2;
    stAioAttr.u32ClkSel = 0;

    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32AencChnCnt = 2;//s32AiChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, HI_FALSE, NULL);
    if (s32Ret != HI_SUCCESS)
    {
        LOTO_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 3: start Aenc
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, gs_enPayloadType);
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

        s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
        if (s32Ret != HI_SUCCESS)
        {
            LOTO_DBG(s32Ret);
            return s32Ret;
        }
        printf("Ai(%d,%d) bind to AencChn:%d ok!\n",AiDev , AiChn, AeChn);
    }

    InitParam initParam;
	initParam.u32AudioSamplerate=48000;
	initParam.ucAudioChannel=2;
	initParam.u32PCMBitSize=16;
	initParam.ucAudioCodec = Law_PCM16;

	g_Easy_H = Easy_AACEncoder_Init(initParam);

    s32Ret = LOTO_AUDIO_CreatTrdAenc(AeChn, NULL);
    if (s32Ret != HI_SUCCESS)
    {
        LOTO_DBG(s32Ret);
        return HI_FAILURE;
    }

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

    return HI_SUCCESS;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_AUDIO_HandleSig(HI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {

        // SAMPLE_COMM_AUDIO_DestoryAllTrd();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

void * LOTO_AENC_AAC_CLASSIC(void *p)
{
    LOTO_AUDIO_AiAenc();
    SAMPLE_COMM_SYS_Exit();

    return  NULL;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */