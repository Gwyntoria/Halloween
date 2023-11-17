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
#include "http_server.h"

#define SNAP_JPEG_FILE ("1.jpg")

HI_S32 LOTO_COMM_VENC_StartGetStream(HI_S32 s32Cnt, pthread_t* vencPid);

HI_S32 LOTO_COMM_VENC_GetSnapJpg(char* jpg, int* jpg_size);

HI_S32 LOTO_VENC_SetVencBitrate(HI_S32 dstBitrate);

HI_S32 LOTO_VENC_LowBitrateMode(CONTROLLER_COVER_STATE cover_state);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif