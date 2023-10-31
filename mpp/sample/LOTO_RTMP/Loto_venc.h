#ifndef LOTO_VENC_H
#define LOTO_VENC_H

#include <pthread.h>
#include "sample_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

HI_S32 LOTO_COMM_VENC_StartGetStream(HI_S32 s32Cnt, pthread_t* vencPid);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif