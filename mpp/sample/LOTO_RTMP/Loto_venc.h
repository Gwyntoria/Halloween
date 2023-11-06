#ifndef LOTO_VENC_H
#define LOTO_VENC_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <pthread.h>

#include "hi_type.h"
#include "hi_common.h"
#include "hi_comm_video.h"

#define SNAP_JPEG_FILE ("1.jpg")

HI_S32 LOTO_COMM_VENC_StartGetStream(HI_S32 s32Cnt, pthread_t* vencPid);

HI_S32 LOTO_COMM_VENC_CreateSnapGroup(VENC_GRP VencGrp, VENC_CHN VencChn, VIDEO_NORM_E enNorm, SIZE_S* pstSize);
HI_S32 LOTO_COMM_VENC_GetSnapJpg(char* jpg, int* jpg_len);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif