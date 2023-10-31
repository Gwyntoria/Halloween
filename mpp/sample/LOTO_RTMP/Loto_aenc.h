#ifndef LOTO_AENC_H
#define LOTO_AENC_H

#include <pthread.h>

#include "sample_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

HI_S32 LOTO_AUDIO_CreatTrdAenc(AENC_CHN AeChn, pthread_t* aencPid);
HI_S32 LOTO_AUDIO_DestoryTrdAenc(AENC_CHN AeChn);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif