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

#include "Loto_venc.h"

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


static pthread_t gs_VencPid;
static SAMPLE_VENC_GETSTREAM_PARA_S gs_stPara;

static gs_snap_group_status = 0;

extern int is_rtmp_write;


/******************************************************************************
* funciton : get stream from each channels and save them
******************************************************************************/
HI_VOID* LOTO_COMM_VENC_GetVencStreamProc(HI_VOID *p)
{
    HI_S32 i;
    HI_S32 s32ChnTotal;
    VENC_CHN_ATTR_S stVencChnAttr;
    SAMPLE_VENC_GETSTREAM_PARA_S *pstPara;
    HI_S32 maxfd = 0;
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 VencFd[VENC_MAX_CHN_NUM];
    HI_CHAR aszFileName[VENC_MAX_CHN_NUM][64];
    FILE *pFile[VENC_MAX_CHN_NUM];
    char szFilePostfix[10];
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
    VENC_CHN VencChn;
    
    pstPara = (SAMPLE_VENC_GETSTREAM_PARA_S*)p;
    s32ChnTotal = pstPara->s32Cnt;

    /******************************************
     step 1:  check & prepare save-file & venc-fd
    ******************************************/
    if (s32ChnTotal >= VENC_MAX_CHN_NUM)
    {
        LOGE("input count invaild\n");
        return NULL;
    }
    for (i = 0; i < s32ChnTotal; i++)
    {
        /* decide the stream file name, and open file to save stream */
        VencChn = i;
        s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
        if(s32Ret != HI_SUCCESS)
        {
            LOGE("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x\n", \
                   VencChn, s32Ret);
            return NULL;
        }

        /* Set Venc Fd. */
        VencFd[i] = HI_MPI_VENC_GetFd(i);
        if (VencFd[i] < 0)
        {
            LOGE("HI_MPI_VENC_GetFd failed with %#x\n", 
                   VencFd[i]);
            return NULL;
        }
        if (maxfd <= VencFd[i])
        {
            maxfd = VencFd[i];
        }
    }

    /******************************************
     step 2:  Start to get streams of each channel.
    ******************************************/

    long long ll_us_sys_timestamp = 0;
    long long ll_first_timestamp = 0;
    char packbuffer[1024 * 1024] = {0};
    int  n_count = 0;
    while (HI_TRUE == pstPara->bThreadStart)
    {
        // while (is_rtmp_write == 1)
        // {
        //     usleep(1000);
        // }
        
        FD_ZERO(&read_fds);
        for (i = 0; i < s32ChnTotal; i++)
        {
            FD_SET(VencFd[i], &read_fds);
        }

        TimeoutVal.tv_sec  = 0;
        TimeoutVal.tv_usec = 1000 * 100;
        s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            LOGE("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            LOGE("get venc stream time out, exit thread\n");
            usleep(500);
            continue;
        }
        else
        {
            for (i = 0; i < s32ChnTotal; i++)
            {
                if (FD_ISSET(VencFd[i], &read_fds))
                {
                    /*******************************************************
                     step 2.1 : query how many packs in one-frame stream.
                    *******************************************************/
                    memset(&stStream, 0, sizeof(stStream));
                    s32Ret = HI_MPI_VENC_Query(i, &stStat);
                    if (HI_SUCCESS != s32Ret)
                    {
                        LOGE("HI_MPI_VENC_Query chn[%d] failed with %#x\n", i, s32Ret);
                        break;
                    }

                    /*******************************************************
                     step 2.2 : malloc corresponding number of pack nodes.
                    *******************************************************/
                    //stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                    // if (NULL == stStream.pstPack)
                    // {
                    //     LOGE("malloc stream pack failed!\n");
                    //     break;
                    // }
                    stStream.pstPack = (VENC_PACK_S *)packbuffer;
                    /*******************************************************
                     step 2.3 : call mpi to get one-frame stream
                    *******************************************************/
                    stStream.u32PackCount = stStat.u32CurPacks;
                    s32Ret = HI_MPI_VENC_GetStream(i, &stStream, HI_TRUE);
                    
                    // if (ll_us_sys_timestamp == 0)
                    // {
                    //     ll_us_sys_timestamp = get_timestamp_us();
                    //     ll_first_timestamp = stStream.pstPack[0].u64PTS;

                    //     // LOGE("[%s] HI_MPI_VENC_GetStream:  ll_us_sys_timestamp = %"PRIu64", ll_first_timestamp = %"PRIu64" \n", log_Time(), ll_us_sys_timestamp, ll_first_timestamp);
                    // }
                    // stStream.pstPack[0].u64PTS = ll_us_sys_timestamp+(stStream.pstPack[0].u64PTS-ll_first_timestamp);

                    // if (n_count == 0 || n_count == 3000)
                    // {
                    //     LOGD("[%s] HI_MPI_VENC_GetStream:  timestamp = %"PRIu64"", log_Time(), stStream.pstPack[0].u64PTS/1000);
                    //     n_count = 0;
                    // }
                    // n_count ++;
                    
                    if (HI_SUCCESS != s32Ret)
                    {
                        //free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        LOGE("HI_MPI_VENC_GetStream failed with %#x\n", \
                               s32Ret);
                        break;
                    }

                    s32Ret = HisiPutH264DataToBuffer(&stStream);
                    if (HI_SUCCESS != s32Ret)
                    {
                        //free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        LOGE("save stream failed!\n");
                        break;
                    }
                    /*******************************************************
                     step 2.5 : release stream
                    *******************************************************/
                    s32Ret = HI_MPI_VENC_ReleaseStream(i, &stStream);
                    if (HI_SUCCESS != s32Ret)
                    {
                        //free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        break;
                    }
                    /*******************************************************
                     step 2.6 : free pack nodes
                    *******************************************************/
                    //free(stStream.pstPack);
                    stStream.pstPack = NULL;
                }
            }
        }
    }

    /*******************************************************
    * step 3 : close save-file
    *******************************************************/
    for (i = 0; i < s32ChnTotal; i++)
    {
        fclose(pFile[i]);
    }

    return NULL;
}

/******************************************************************************
* funciton : start get venc stream process thread
******************************************************************************/
HI_S32 LOTO_COMM_VENC_StartGetStream(HI_S32 s32Cnt, pthread_t* vencPid)
{
    gs_stPara.bThreadStart = HI_TRUE;
    gs_stPara.s32Cnt = s32Cnt;

    HI_S32 nRet = pthread_create(&gs_VencPid, 0, LOTO_COMM_VENC_GetVencStreamProc, (HI_VOID*)&gs_stPara);

    if (nRet == 0)
    {
        if (vencPid != NULL)
            *vencPid = gs_VencPid;
        else
            pthread_join(gs_VencPid, 0);
    }

    return  nRet;
}

HI_S32 LOTO_COMM_VENC_CreateSnapGroup(VENC_GRP VencGrp, VENC_CHN VencChn, VIDEO_NORM_E enNorm, SIZE_S* pstSize) {
    HI_S32                s32Ret;
    VENC_CHN_ATTR_S       stVencChnAttr;
    VENC_ATTR_JPEG_S      stJpegAttr;


    stVencChnAttr.stVeAttr.enType = PT_JPEG;

    stJpegAttr.u32MaxPicWidth  = pstSize->u32Width;
    stJpegAttr.u32MaxPicHeight = pstSize->u32Height;
    stJpegAttr.u32PicWidth     = pstSize->u32Width;
    stJpegAttr.u32PicHeight    = pstSize->u32Height;
    stJpegAttr.u32BufSize      = pstSize->u32Width * pstSize->u32Height * 2;
    stJpegAttr.bByFrame        = HI_TRUE;  /*get stream mode is field mode  or frame mode*/
    stJpegAttr.bVIField        = HI_FALSE; /*the sign of the VI picture is field or frame?*/
    stJpegAttr.u32Priority     = 0;        /*channels precedence level*/
    memcpy(&stVencChnAttr.stVeAttr.stAttrJpeg, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));

    s32Ret = HI_MPI_VENC_CreateGroup(VencGrp);
    if (HI_SUCCESS != s32Ret) {
        LOGE("HI_MPI_VENC_CreateGroup[%d] failed with %#x!\n", VencGrp, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
    if (HI_SUCCESS != s32Ret) {
        LOGE("HI_MPI_VENC_CreateChn [%d] faild with %#x!\n", VencChn, s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VENC_RegisterChn(VencGrp, VencChn);
    if (HI_SUCCESS != s32Ret) {
        LOGE("HI_MPI_VENC_RegisterChn faild with %#x\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_StartRecvPic(VencChn);
    if (HI_SUCCESS != s32Ret) {
        LOGE("HI_MPI_VENC_StartRecvPic faild with%#x\n", s32Ret);
        return HI_FAILURE;
    }

    gs_snap_group_status = 1;

    return HI_SUCCESS;
}

HI_S32 LOTO_COMM_VENC_SaveSnap(VENC_STREAM_S* pstStream) {
    HI_S32 s32Ret;

    char         acFile[128] = {0};
    FILE*        pFile;
    VENC_PACK_S* pstData;
    HI_U32       i;

    sprintf(acFile, SNAP_JPEG_FILE);
    pFile = fopen(acFile, "wb");
    if (pFile == NULL) {
        LOGE("open file err\n");
        return HI_FAILURE;
    }

    for (i = 0; i < pstStream->u32PackCount; i++) {
        pstData = &pstStream->pstPack[i];
        fwrite(pstData->pu8Addr[0], pstData->u32Len[0], 1, pFile);
        fwrite(pstData->pu8Addr[1], pstData->u32Len[1], 1, pFile);
    }

    fclose(pFile);

    return HI_SUCCESS;
}

HI_S32 LOTO_COMM_VENC_GetSnapJpg() {
    if (gs_snap_group_status != 1) 
        return HI_FAILURE;

    HI_S32 s32Ret;

    struct timeval  TimeoutVal;
    fd_set          read_fds;
    HI_S32          s32VencFd;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S   stStream;
    HI_S32          i;

    VENC_CHN VencChn = 1;

    s32VencFd = HI_MPI_VENC_GetFd(VencChn);
    if (s32VencFd < 0) {
        LOGE("HI_MPI_VENC_GetFd faild with %#x\n", s32VencFd);
        return HI_FAILURE;
    }

    FD_ZERO(&read_fds);
    FD_SET(s32VencFd, &read_fds);

    TimeoutVal.tv_sec  = 0;
    TimeoutVal.tv_usec = 1000 * 100;

    s32Ret = select(s32VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0) {
        LOGE("snap select failed!\n");
        return HI_FAILURE;

    } else if (0 == s32Ret) {
        LOGE("snap time out!\n");
        return HI_FAILURE;

    } else {
        if (FD_ISSET(s32VencFd, &read_fds)) {
            s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
            if (s32Ret != HI_SUCCESS) {
                LOGE("HI_MPI_VENC_Query failed with %#x\n", s32Ret);
                return HI_FAILURE;
            }

            stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
            if (NULL == stStream.pstPack) {
                LOGE("malloc memory failed!\n");
                return HI_FAILURE;
            }

            stStream.u32PackCount = stStat.u32CurPacks;

            s32Ret = HI_MPI_VENC_GetStream(VencChn, &stStream, HI_TRUE);
            if (HI_SUCCESS != s32Ret) {
                LOGE("HI_MPI_VENC_GetStream failed with %#x\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            s32Ret = LOTO_COMM_VENC_SaveSnap(&stStream);
            if (HI_SUCCESS != s32Ret) {
                LOGE("HI_MPI_VENC_GetStream failed with %#x\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            s32Ret = HI_MPI_VENC_ReleaseStream(VencChn, &stStream);
            if (s32Ret) {
                LOGE("HI_MPI_VENC_ReleaseStream failed with %#x\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            free(stStream.pstPack);
            stStream.pstPack = NULL;
        }
    }

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */