/*
 * WaInit.h:
 *
 ***********************************************************************
 * by Jessica Mao
 * Lotogram Inc,. 2020/04/17
 *
 ***********************************************************************
 */

#ifndef WAINIT_H
#define WAINIT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

loto_room_info* loto_room_init();
cJSON* get_frp_info();

#ifdef __cplusplus
}
#endif

#endif
