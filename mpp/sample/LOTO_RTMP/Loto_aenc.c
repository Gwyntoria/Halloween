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

#include "Loto_aenc.h"

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
// #include "EasyAACEncoderAPI.h"
#include "faac.h"
#include "fdk-aac/aacenc_lib.h"
#include "ConfigParser.h"

#define AAC_ENC_CHANNAL_COUNT (2)
#define AAC_ENC_SAMPLERATE    (48000)
#define AAC_ENC_FRAME_SIZE    (1024)
#define AAC_ENC_BITRATE       (48000)

#define SAMPLE_AUDIO_PTNUMPERFRM (1024)
#define SAMPLE_AUDIO_AI_DEV      (0)

typedef struct tagLOTO_AENC_S {
    HI_BOOL   bStart;
    pthread_t stAencPid;
    HI_S32    AeChn;
} LOTO_AENC_S;

static LOTO_AENC_S gs_stLotoAenc[AENC_MAX_CHN_NUM];

/******************************************************************************
* function : get stream from Aenc, send it  to ringfifo
******************************************************************************/
void* LOTO_COMM_AUDIO_AencProc(void* parg)
{
    HI_S32 s32Ret;
    HI_S32 AencFd[2];
    HI_S32 maxfd = 0;
    int    i = 0, k = 0;

    LOTO_AENC_S*   pstAencCtl = (LOTO_AENC_S*)parg;
    AUDIO_STREAM_S stStream;
    fd_set         read_fds;
    struct timeval TimeoutVal;

    HI_U64 lastTimeStamp = 0;

    HANDLE_AACENCODER g_Enc_H = NULL;

    int aac_enc_chn = AAC_ENC_CHANNAL_COUNT;

    if (aacEncOpen(&g_Enc_H, 0, 2) != AACENC_OK)
    {
        printf("[ERROR] Failed to call aacEncOpen()\n");
        return  NULL;
    }

    if (aacEncoder_SetParam(g_Enc_H, AACENC_AOT, 2)!= AACENC_OK)
    {
        aacEncClose(&g_Enc_H);
        printf("[ERROR] Unable to set the AOT\n");
        return  NULL;
    }

    // aacEncoder_SetParam(g_Enc_H, AACENC_BITRATEMODE, 4);
    if (aacEncoder_SetParam(g_Enc_H, AACENC_BITRATE, AAC_ENC_BITRATE) != AACENC_OK)
    {
        aacEncClose(&g_Enc_H);
        printf("[ERROR] Unable to set the Bitrate\n");
        return  NULL;
    }

    if (aacEncoder_SetParam(g_Enc_H, AACENC_SAMPLERATE, AAC_ENC_SAMPLERATE) != AACENC_OK)
    {
        aacEncClose(&g_Enc_H);
        printf("[ERROR] Unable to set the SampleRate\n");
        return  NULL;
    }

    if (aacEncoder_SetParam(g_Enc_H, AACENC_GRANULE_LENGTH, AAC_ENC_FRAME_SIZE) != AACENC_OK)
    {
        aacEncClose(&g_Enc_H);
        printf("[ERROR] Unable to set the Granule length\n");
        return  NULL;
    }

    CHANNEL_MODE channelMode;

    if (aac_enc_chn == 1){
        channelMode = MODE_1;
    } else if (aac_enc_chn == 2) {
        channelMode = MODE_2;
    }

    if (aacEncoder_SetParam(g_Enc_H, AACENC_CHANNELMODE, channelMode) != AACENC_OK)
    {
        aacEncClose(&g_Enc_H);
        printf("[ERROR] Unable to set the ChannelMode\n");
        return  NULL;
    }

    if (aacEncoder_SetParam(g_Enc_H, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK)
    {
        aacEncClose(&g_Enc_H);
        printf("[ERROR] Unable to set the TransMux\n");
        return  NULL;
    }

    if (aacEncEncode(g_Enc_H, NULL, NULL, NULL, NULL) != AACENC_OK)
    {
        aacEncClose(&g_Enc_H);
        printf("[ERROR] Unable to initialize the encoder\n");
        return  NULL;
    }

    // char szServerFile[256] = "";
	// sprintf(szServerFile, "%s/server.ini", WORK_FOLDER);
    // int i_writefile = GetIniKeyInt((char*)"server", (char*)"writefile", szServerFile);

    for (i = 0; i < aac_enc_chn; i++)
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

    }

    AACENC_BufDesc in_buf_des    = {0};
    AACENC_BufDesc out_buf_des   = {0};
    AACENC_InArgs  in_args       = {0};
    AACENC_OutArgs out_args      = {0};
    int            in_identifier = IN_AUDIO_DATA;
    int            in_size, in_elem_size;
    int            out_identifier = OUT_BITSTREAM_DATA;
    int            out_size, out_elem_size;
    void*          in_ptr;
    void*          out_ptr;
    INT_PCM        inbuf[AAC_ENC_FRAME_SIZE * aac_enc_chn];
    uint8_t        outbuf[AAC_ENC_FRAME_SIZE * 2];

    in_size      = AAC_ENC_FRAME_SIZE * 2 * aac_enc_chn;
    in_ptr       = inbuf;
    in_elem_size = 2;

    in_buf_des.numBufs           = 1;
    in_buf_des.bufs              = &in_ptr;
    in_buf_des.bufferIdentifiers = &in_identifier;
    in_buf_des.bufSizes          = &in_size;
    in_buf_des.bufElSizes        = &in_elem_size;

    out_ptr                       = outbuf;
    out_size                      = sizeof(outbuf);
    out_elem_size                 = 1;
    out_buf_des.numBufs           = 1;
    out_buf_des.bufs              = &out_ptr;
    out_buf_des.bufferIdentifiers = &out_identifier;
    out_buf_des.bufSizes          = &out_size;
    out_buf_des.bufElSizes        = &out_elem_size;

    in_args.numInSamples = AAC_ENC_FRAME_SIZE * 2 * aac_enc_chn;

    while (pstAencCtl->bStart)
    {
        // FD_ZERO(&read_fds);
        // FD_SET(AencFd,&read_fds); 

        FD_ZERO(&read_fds);
        for (i = 0; i < aac_enc_chn; i++)
        {
            FD_SET(AencFd[i], &read_fds);
        }
    
        TimeoutVal.tv_sec = 0;
        TimeoutVal.tv_usec = 1000 * 50;

        s32Ret = select(maxfd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            break;
        }
        else if (0 == s32Ret)
        {
            printf("%s: get aenc stream select time out\n", __FUNCTION__);
            // usleep(500);
            continue;
        }

        // memset(aacBuf, 0, sizeof(aacBuf));
        // memset(streamBuf, 0, 1024*5);

        for (i = 0; i < aac_enc_chn; i ++)
        {
            int st = i;
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

                for (k = 0; k < stStream.u32Len; k += 2) {
                    inbuf[st] = (stStream.pStream[k]) | stStream.pStream[k + 1] << 8;
                    st += aac_enc_chn;
                }

                HI_MPI_AENC_ReleaseStream(i, &stStream);                
            }
        }

        if (aacEncEncode(g_Enc_H, &in_buf_des, &out_buf_des, &in_args, &out_args) ==
            AACENC_OK) {
            if (out_args.numOutBytes > 0) {
                lastTimeStamp = stStream.u64TimeStamp;
                // printf("aac timestamp: %llu\n", lastTimeStamp);
                HisiPutAACDataToBuffer(outbuf, out_args.numOutBytes, lastTimeStamp, 0);

                // printf("%s: aac length = %d \n", __FUNCTION__, out_args.numOutBytes);

                // load_data_stream_hex(outbuf, out_args.numOutBytes);
            } else {
                printf("%s: Encoded 0 bytes\n", __FUNCTION__);
            }
        } else {
            printf("%s: Encoding failed\n", __FUNCTION__);
        }

        memset(inbuf, 0, sizeof(inbuf));
        memset(outbuf, 0, sizeof(outbuf));
    }

    aacEncClose(&g_Enc_H);

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

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */