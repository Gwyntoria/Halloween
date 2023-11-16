#include "loto_cover.h"

#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hi_comm_region.h"
#include "hi_common.h"
#include "hi_type.h"

#include "common.h"
#include "loto_osd.h"
#include "Loto_venc.h"
#include "ringfifo.h"
#include "http_server.h"
#include "ConfigParser.h"

extern DeviceInfo g_device_info;

static RGN_HANDLE gs_rgnHandle = 5;

static MPP_CHN_S gs_stMppChnAttr = {
    .enModId  = HI_ID_VIU,
    .s32DevId = 0,
    .s32ChnId = 0,
};

static RGN_CHN_ATTR_S gs_stRgnChnAttr = {
    .bShow                                 = HI_TRUE,
    .enType                                = COVER_RGN,
    .unChnAttr.stCoverChn.stRect.s32X      = 0,
    .unChnAttr.stCoverChn.stRect.s32Y      = 0,
    .unChnAttr.stCoverChn.stRect.u32Width  = 1280,
    .unChnAttr.stCoverChn.stRect.u32Height = 720,
    .unChnAttr.stCoverChn.u32Color         = 0x202020,
    .unChnAttr.stCoverChn.u32Layer         = 0,
};

HI_S32 LOTO_COVER_InitCoverRegion() {

    HI_S32     ret       = 0;
    RGN_ATTR_S stRgnAttr = {0};
    stRgnAttr.enType     = COVER_RGN;

    ret = HI_MPI_RGN_Create(gs_rgnHandle, &stRgnAttr);
    if (ret != HI_SUCCESS) {
        LOGE("HI_MPI_RGN_Create failed with %#x\n", ret);
        return HI_FAILURE;
    }

    LOGI("Create Cover region success!\n");

    return ret;
}

HI_S32 LOTO_COVER_UninitCoverRegion() {
    HI_S32 ret;

    ret = HI_MPI_RGN_Destroy(gs_rgnHandle);
    if (ret != HI_SUCCESS) {
        LOGE("HI_MPI_RGN_Destroy failed with %#x!\n", ret);
        return HI_FAILURE;
    }

    return ret;
}

HI_S32 LOTO_COVER_AddCover() {
    HI_S32 ret = 0;

    ret = HI_MPI_RGN_AttachToChn(gs_rgnHandle, &gs_stMppChnAttr, &gs_stRgnChnAttr);
    if (ret != HI_SUCCESS) {
        LOGE("HI_MPI_RGN_AttachToChn failed with %#x!\n", ret);
        return HI_FAILURE;
    }

    if (LOTO_VENC_SetVencBitrate(8) != HI_SUCCESS) {
        LOGE("LOTO_VENC_SetVencBitrate failed!\n");
        return HI_FAILURE;
    }

    // LOGI("add cover!\n");

    return HI_SUCCESS;
}

HI_S32 LOTO_COVER_RemoveCover() {
    HI_S32 ret = 0;

    ret = HI_MPI_RGN_DetachFrmChn(gs_rgnHandle, &gs_stMppChnAttr);
    if (ret != HI_SUCCESS) {
        LOGE("HI_MPI_RGN_DetachFromChn failed with %#x\n", ret);
        return HI_FAILURE;
    }

    if (LOTO_VENC_SetVencBitrate(4096) != HI_SUCCESS) {
        LOGE("LOTO_VENC_SetVencBitrate failed!\n");
        return HI_FAILURE;
    }

    // LOGI("remove cover!\n");

    return HI_SUCCESS;
}

static int gs_cover_switch = 0;
static int gs_cover_state  = COVER_OFF;

int LOTO_COVER_GetCoverState() {
    return gs_cover_state;
}

void LOTO_COVER_Switch(int state) {
    if (gs_cover_state != state) {
        gs_cover_switch = 1;
        gs_cover_state  = state;
    }
}

HI_S32 LOTO_COVER_ChangeCover() {
    if (gs_cover_switch == 0) {
        return 0;
    }

    if (gs_cover_state == COVER_ON) {
        if (LOTO_COVER_AddCover() != 0) {
            LOGE("LOTO_COVER_AddCover failed!\n");
            gs_cover_state = COVER_OFF;
            gs_cover_switch = 0;
            return -1;
        }

        g_device_info.video_state = COVER_ON;
        PutConfigKeyValue("push", "video_state", "off", PUSH_CONFIG_FILE_PATH);

    } else if (gs_cover_state == COVER_OFF) {
        if (LOTO_COVER_RemoveCover() != 0) {
            LOGE("LOTO_COVER_RemoveCover failed!\n");
            gs_cover_state = COVER_ON;
            gs_cover_switch = 0;
            return -1;
        }

        g_device_info.video_state = COVER_OFF;
        PutConfigKeyValue("push", "video_state", "on", PUSH_CONFIG_FILE_PATH);
    }

    gs_cover_switch = 0;

    return 0;
}
