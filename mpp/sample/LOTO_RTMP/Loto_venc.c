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

#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "common.h"
#include "ringfifo.h"
#include "sample_comm.h"


typedef struct Cache {
    char data[1024 * 1024];
    int  data_len;
} Cache;

static pthread_t                    gs_VencPid;
static SAMPLE_VENC_GETSTREAM_PARA_S gs_stPara;

int gs_snap_group_status = 0;

extern int is_rtmp_write;

static char packBuffer[1024 * 1024] = {0};

static Cache cache_1 = {0};
static Cache cache_2 = {0};

int cur_write_buffer = 0;
int is_writing       = 0;

HI_S32 LOTO_VENC_WriteMJpeg(VENC_STREAM_S* pstStream) {
    VENC_PACK_S* pstData     = NULL;
    Cache*       write_cache = NULL;
    char*        buf         = NULL;

    is_writing = 1;
    // printf("write_buffer:       %d\n", cur_write_buffer);

    if (cur_write_buffer == 0) {
        cur_write_buffer = 1;
        write_cache      = &cache_1;
    } else if (cur_write_buffer == 1) {
        write_cache = &cache_1;
    } else if (cur_write_buffer == 2) {
        write_cache = &cache_2;
    }

    if (write_cache == NULL) {
        LOGE("write_cache is NULL!\n");
        return -1;
    }

    memset(write_cache, 0, sizeof(Cache));

    buf = write_cache->data;

    if (buf == NULL) {
        LOGE("buf is NULL!\n");
        return -1;
    }

    int  i   = 0;
    int* len = &(write_cache->data_len);
    for (i = 0; i < pstStream->u32PackCount; i++) {
        pstData = &pstStream->pstPack[i];

        memcpy(buf, pstData->pu8Addr[0], pstData->u32Len[0]);
        buf += pstData->u32Len[0];
        *len += pstData->u32Len[0];
        // printf("pstData->u32Len[0] = %d\n", pstData->u32Len[0]);

        memcpy(buf, pstData->pu8Addr[1], pstData->u32Len[1]);
        buf += pstData->u32Len[1];
        *len += pstData->u32Len[1];
        // printf("pstData->u32Len[1] = %d\n", pstData->u32Len[1]);
    }

    // printf("data_len:           %d\n", write_cache->data_len);

    is_writing = 0;

    return 0;
}

HI_S32 LOTO_VENC_ReadMJpeg(char* jpg, int* jpg_size) {
    while (is_writing) {
        usleep(100);
    }

    // printf("read_buffer:        %d\n", cur_write_buffer);

    cur_write_buffer = (cur_write_buffer == 1) ? 2 : 1;

    Cache* read_cache = NULL;

    if (cur_write_buffer == 2) {
        read_cache = &cache_1;
    } else if (cur_write_buffer == 1) {
        read_cache = &cache_2;
    }

    if (read_cache == NULL) {
        LOGE("read_cache is NULL\n");
        return -1;
    }

    *jpg_size = read_cache->data_len;
    if (*jpg_size > 0) {
        memcpy(jpg, read_cache->data, *jpg_size);
        memset(read_cache, 0, sizeof(Cache));

    } else {
        return -1;
    }

    return 0;
}

/******************************************************************************
 * funciton : get stream from each channels and save them
 ******************************************************************************/
HI_VOID* LOTO_COMM_VENC_GetVencStreamProc(HI_VOID* p) {
    HI_S32                        i;
    HI_S32                        s32ChnTotal;
    SAMPLE_VENC_GETSTREAM_PARA_S* pstPara;
    HI_S32                        maxfd = 0;
    struct timeval                TimeoutVal;
    fd_set                        read_fds;
    HI_S32                        VencFd[VENC_MAX_CHN_NUM];
    HI_CHAR                       aszFileName[VENC_MAX_CHN_NUM][64];
    FILE*                         pFile[VENC_MAX_CHN_NUM];
    char                          szFilePostfix[10];
    VENC_CHN_STAT_S               stStat;
    VENC_STREAM_S                 stStream;
    HI_S32                        s32Ret;
    VENC_CHN                      VencChn;

    pstPara     = (SAMPLE_VENC_GETSTREAM_PARA_S*)p;
    s32ChnTotal = pstPara->s32Cnt;

    /******************************************
     step 1:  check & prepare save-file & venc-fd
    ******************************************/
    if (s32ChnTotal >= VENC_MAX_CHN_NUM) {
        LOGE("input count invaild\n");
        return NULL;
    }

    for (i = 0; i < s32ChnTotal; i++) {
        VENC_CHN VencChn = i;

        /* Set Venc Fd. */
        VencFd[i] = HI_MPI_VENC_GetFd(VencChn);
        if (VencFd[i] < 0) {
            LOGE("HI_MPI_VENC_GetFd failed with %#x\n", VencFd[i]);
            return NULL;
        }
        if (maxfd <= VencFd[i]) {
            maxfd = VencFd[i];
        }
    }

    /******************************************
     step 2:  Start to get streams of each channel.
    ******************************************/
    while (HI_TRUE == pstPara->bThreadStart) {
        FD_ZERO(&read_fds);
        for (i = 0; i < s32ChnTotal; i++) {
            FD_SET(VencFd[i], &read_fds);
        }

        TimeoutVal.tv_sec  = 0;
        TimeoutVal.tv_usec = 1000 * 300;

        s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0) {
            LOGE("select failed!\n");
            break;
        } else if (s32Ret == 0) {
            LOGE("get venc stream time out, exit thread\n");
            usleep(500);
            continue;
        } else {
            for (i = 0; i < s32ChnTotal; i++) {
                VENC_CHN VencChn = i;

                if (FD_ISSET(VencFd[i], &read_fds)) {
                    /*******************************************************
                     step 2.1 : query how many packs in one-frame stream.
                    *******************************************************/
                    memset(&stStream, 0, sizeof(stStream));
                    s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
                    if (HI_SUCCESS != s32Ret) {
                        LOGE("HI_MPI_VENC_Query chn[%d] failed with %#x\n", i, s32Ret);
                        break;
                    }

                    /*******************************************************
                     step 2.2 : malloc corresponding number of pack nodes.
                    *******************************************************/
                    stStream.pstPack = (VENC_PACK_S*)packBuffer;
                    // stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                    // if (NULL == stStream.pstPack)
                    // {
                    //     LOGE("malloc stream pack failed!\n");
                    //     break;
                    // }
                    /*******************************************************
                     step 2.3 : call mpi to get one-frame stream
                    *******************************************************/
                    stStream.u32PackCount = stStat.u32CurPacks;

                    s32Ret = HI_MPI_VENC_GetStream(i, &stStream, HI_TRUE);
                    if (HI_SUCCESS != s32Ret) {
                        // free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        LOGE("HI_MPI_VENC_GetStream failed with %#x\n", s32Ret);
                        break;
                    }

                    if (i == 0) {
                        s32Ret = HisiPutH264DataToBuffer(&stStream);
                        if (HI_SUCCESS != s32Ret) {
                            // free(stStream.pstPack);
                            stStream.pstPack = NULL;
                            LOGE("save stream failed!\n");
                            // break;
                        }

                    } else {
                        s32Ret = LOTO_VENC_WriteMJpeg(&stStream);
                        if (HI_SUCCESS != s32Ret) {
                            // free(stStream.pstPack);
                            stStream.pstPack = NULL;
                            LOGE("save jpeg failed!\n");
                            // break;
                        }
                    }
                    /*******************************************************
                     step 2.5 : release stream
                    *******************************************************/
                    s32Ret = HI_MPI_VENC_ReleaseStream(VencChn, &stStream);
                    if (HI_SUCCESS != s32Ret) {
                        // free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        break;
                    }
                    /*******************************************************
                     step 2.6 : free pack nodes
                    *******************************************************/
                    // free(stStream.pstPack);
                    stStream.pstPack = NULL;
                }

                memset(packBuffer, 0, sizeof(packBuffer));
            }
        }
    }

    return NULL;
}

/******************************************************************************
 * funciton : start get venc stream process thread
 ******************************************************************************/
HI_S32 LOTO_COMM_VENC_StartGetStream(HI_S32 s32Cnt, pthread_t* vencPid) {
    gs_stPara.bThreadStart = HI_TRUE;
    gs_stPara.s32Cnt       = s32Cnt;

    HI_S32 nRet = pthread_create(&gs_VencPid, 0, LOTO_COMM_VENC_GetVencStreamProc, (HI_VOID*)&gs_stPara);

    if (nRet == 0) {
        if (vencPid != NULL)
            *vencPid = gs_VencPid;
        else
            pthread_join(gs_VencPid, 0);
    }

    return nRet;
}

HI_S32 LOTO_COMM_VENC_SaveSnap(VENC_STREAM_S* pstStream, char* jpg, int* jpg_size) {
    // char         acFile[128] = {0};
    // FILE*        pFile;

    // sprintf(acFile, SNAP_JPEG_FILE);
    // pFile = fopen(acFile, "wb");
    // if (pFile == NULL) {
    //     LOGE("open file err\n");
    //     return HI_FAILURE;
    // }

    HI_U32       i;
    VENC_PACK_S* pstData;
    char*        buffer_index = jpg;

    for (i = 0; i < pstStream->u32PackCount; i++) {
        pstData = &pstStream->pstPack[i];
        // fwrite(pstData->pu8Addr[0], pstData->u32Len[0], 1, pFile);
        // fwrite(pstData->pu8Addr[1], pstData->u32Len[1], 1, pFile);

        memcpy(buffer_index, pstData->pu8Addr[0], pstData->u32Len[0]);
        buffer_index += pstData->u32Len[0];
        *jpg_size += pstData->u32Len[0];

        memcpy(buffer_index, pstData->pu8Addr[1], pstData->u32Len[1]);
        buffer_index += pstData->u32Len[1];
        *jpg_size += pstData->u32Len[1];
    }

    // fclose(pFile);

    return HI_SUCCESS;
}

// static char mjpegPackBuf[1024 * 1024] = {0};

HI_S32 LOTO_COMM_VENC_GetSnapJpg(char* jpg, int* jpg_size) {
    if (gs_snap_group_status != 1) {
        LOGE("snap venc group has not yet been created!\n");
        return HI_FAILURE;
    }

    HI_S32 s32Ret;

    s32Ret = LOTO_VENC_ReadMJpeg(jpg, jpg_size);
    if (s32Ret != 0) {
        LOGE("LOTO_VENC_ReadMJpeg failed!\n");
        return HI_FAILURE;
    }

    /*
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
    TimeoutVal.tv_usec = 1000 * 300;

    s32Ret = select(s32VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0) {
        LOGE("snap select failed!\n");
        return HI_FAILURE;

    } else if (0 == s32Ret) {
        LOGE("snap time out!\n");
        return HI_FAILURE;
    } else {

        if (FD_ISSET(s32VencFd, &read_fds)) {
            memset(&stStream, 0, sizeof(stStream));

            s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
            if (s32Ret != HI_SUCCESS) {
                LOGE("HI_MPI_VENC_Query failed with %#x\n", s32Ret);
                return HI_FAILURE;
            }

            stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
            // stStream.pstPack = (VENC_PACK_S*)mjpegPackBuf;
            if (NULL == stStream.pstPack) {
                LOGE("malloc memory failed!\n");
                return HI_FAILURE;
            }

            stStream.u32PackCount = stStat.u32CurPacks;

            s32Ret = HI_MPI_VENC_GetStream(VencChn, &stStream, HI_TRUE);
            if (HI_SUCCESS != s32Ret) {
                LOGE("HI_MPI_VENC_GetStream failed with %#x\n", s32Ret);
                free(stStream.pstPack);
                // memset(mjpegPackBuf, 0, sizeof(mjpegPackBuf));
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            s32Ret = LOTO_COMM_VENC_SaveSnap(&stStream, jpg, jpg_size);
            if (HI_SUCCESS != s32Ret) {
                LOGE("HI_MPI_VENC_GetStream failed with %#x\n", s32Ret);
                free(stStream.pstPack);
                // memset(mjpegPackBuf, 0, sizeof(mjpegPackBuf));
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            s32Ret = HI_MPI_VENC_ReleaseStream(VencChn, &stStream);
            if (s32Ret) {
                LOGE("HI_MPI_VENC_ReleaseStream failed with %#x\n", s32Ret);
                free(stStream.pstPack);
                // memset(mjpegPackBuf, 0, sizeof(mjpegPackBuf));
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            free(stStream.pstPack);
            // memset(mjpegPackBuf, 0, sizeof(mjpegPackBuf));
            stStream.pstPack = NULL;
        }
    } */

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */