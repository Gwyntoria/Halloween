/**
 * @file Loto_osd.c
 * @copyright 2022, Lotogram Tech. Co., Ltd.
 * @brief Add video OSD on hi3516dv300
 * @version 0.1
 * @date 2022-10-19
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "loto_osd.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "hi_comm_video.h"
#include "sample_comm.h"

#define OSD_HANDLE_DEVICENUM 1
#define OSD_HANDLE_TIMESTAMP 2

#define FILE_COUNT 12

extern char g_device_num[16];

static HI_U8*               bmpBuffer[FILE_COUNT];
static OSD_BITMAPFILEHEADER gs_bmpFileHeader[FILE_COUNT];
static OSD_BITMAPINFO       gs_bmpInfo[FILE_COUNT];

typedef struct Pixel_1555 {
    unsigned short blue : 5;
    unsigned short green : 5;
    unsigned short red : 5;
    unsigned short alpha : 1;
} Pixel_1555;

OSD_COMP_INFO s_OSDCompInfo[OSD_COLOR_FMT_BUTT] = {
    {0, 4, 4, 4}, /*RGB444*/
    {4, 4, 4, 4}, /*ARGB4444*/
    {0, 5, 5, 5}, /*RGB555*/
    {0, 5, 6, 5}, /*RGB565*/
    {1, 5, 5, 5}, /*ARGB1555*/
    {0, 0, 0, 0}, /*RESERVED*/
    {0, 8, 8, 8}, /*RGB888*/
    {8, 8, 8, 8}  /*ARGB8888*/
};

HI_U16 OSD_MAKECOLOR_U16(HI_U8 r, HI_U8 g, HI_U8 b, OSD_COMP_INFO compinfo) {
    HI_U8  r1, g1, b1;
    HI_U16 pixel = 0;
    HI_U32 tmp   = 15;

    r1 = g1 = b1 = 0;
    r1           = r >> (8 - compinfo.rlen);
    g1           = g >> (8 - compinfo.glen);
    b1           = b >> (8 - compinfo.blen);
    while (compinfo.alen) {
        pixel |= (1 << tmp);
        tmp--;
        compinfo.alen--;
    }

    pixel |=
        (r1 | (g1 << compinfo.blen) | (b1 << (compinfo.blen + compinfo.glen)));
    return pixel;
}

HI_U16 OSD_MAKECOLOR_ALPHA_U32(HI_U8 a, HI_U8 r, HI_U8 g, HI_U8 b,
                               OSD_COMP_INFO compinfo) {
    HI_U8  r1, g1, b1;
    HI_U16 pixel = 0;

    r1 = g1 = b1 = 0;
    r1           = r >> (8 - compinfo.rlen);
    g1           = g >> (8 - compinfo.glen);
    b1           = b >> (8 - compinfo.blen);
    if (a > 10) {
        pixel = 0x8000;
    }

    pixel |=
        (r1 | (g1 << compinfo.blen) | (b1 << (compinfo.blen + compinfo.glen)));
    return pixel;
}

/**
 * @brief Create OSD region
 *
 * @param handle region's handle (one & only)
 * @return HI_S32 Errors Codes
 */
HI_S32 LOTO_OSD_REGION_Create(RGN_HANDLE handle, HI_U32 canvasWidth,
                              HI_U32 canvasHeight) {
    HI_S32     s32Ret;
    RGN_ATTR_S stRegion;

    // /* OverlayEx */
    stRegion.enType                              = OVERLAYEX_RGN;
    stRegion.unAttr.stOverlayEx.enPixelFmt       = PIXEL_FORMAT_RGB_1555;
    stRegion.unAttr.stOverlayEx.u32BgColor       = 0b00;
    stRegion.unAttr.stOverlayEx.stSize.u32Width  = canvasWidth;
    stRegion.unAttr.stOverlayEx.stSize.u32Height = canvasHeight;

    s32Ret = HI_MPI_RGN_Create(handle, &stRegion);
    if (s32Ret != HI_SUCCESS) {
        LOGE("HI_MPI_RGN_Create failed with %#x\n", s32Ret);

        if (OSD_HANDLE_DEVICENUM == handle) {
            LOGE("Create Device_Num OSD region failed!\n");

        } else if (OSD_HANDLE_TIMESTAMP == handle) {
            LOGE("Create Timestamp OSD region failed!\n");
        }

        return HI_FAILURE;
    }

    if (OSD_HANDLE_DEVICENUM == handle) {
        LOGI("Create Device_Num OSD region success!\n");

    } else if (OSD_HANDLE_TIMESTAMP == handle) {
        LOGI("Create Timestamp OSD region success!\n");
    }

    return HI_SUCCESS;
}

HI_S32 LOTO_OSD_REGION_Destroy(RGN_HANDLE handle) {
    HI_S32 s32Ret;

    s32Ret = HI_MPI_RGN_Destroy(handle);
    if (s32Ret != HI_SUCCESS) {
        LOGE("HI_MPI_RGN_Destroy failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }
    LOGE("Destroy OSD region success!\n");
    return HI_SUCCESS;
}

HI_S32 LOTO_OSD_REGION_AttachToChn(RGN_HANDLE handle, HI_S32 canvasLocX,
                                   HI_S32 canvasLocY) {
    HI_S32         s32Ret;
    MPP_CHN_S      stOverlayExChn;
    RGN_CHN_ATTR_S stOverlayExChnAttr;

    /* Overlay */
    stOverlayExChn.enModId  = HI_ID_VIU;
    stOverlayExChn.s32DevId = 0;
    stOverlayExChn.s32ChnId = 0;

    memset(&stOverlayExChnAttr, 0, sizeof(stOverlayExChnAttr));
    stOverlayExChnAttr.bShow                                 = HI_TRUE;
    stOverlayExChnAttr.enType                                = OVERLAYEX_RGN;
    stOverlayExChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X = canvasLocX;
    stOverlayExChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y = canvasLocY;
    stOverlayExChnAttr.unChnAttr.stOverlayExChn.u32BgAlpha   = 50;
    stOverlayExChnAttr.unChnAttr.stOverlayExChn.u32FgAlpha   = 130;
    stOverlayExChnAttr.unChnAttr.stOverlayExChn.u32Layer     = 1;

    s32Ret =
        HI_MPI_RGN_AttachToChn(handle, &stOverlayExChn, &stOverlayExChnAttr);
    if (HI_SUCCESS != s32Ret) {
        LOGE("HI_MPI_RGN_AttachToChn failed with %#x!\n", s32Ret);
        LOTO_OSD_REGION_Destroy(handle);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 LOTO_OSD_CheckBmpInfo(OSD_BITMAPINFO* pBmpInfo) {
    HI_U16 Bpp;

    Bpp = pBmpInfo->bmiHeader.biBitCount / 8;
    if (Bpp < 2) {
        /* only support 1555.8888  888 bitmap */
        LOGE("bitmap format not supported!\n");
        return -1;
    }

    if (pBmpInfo->bmiHeader.biCompression != 0) {
        LOGE("not support compressed bitmap file!\n");
        LOGE("bitCompression:%d\n", pBmpInfo->bmiHeader.biCompression);
        return -1;
    }

    if (pBmpInfo->bmiHeader.biHeight < 0) {
        LOGE("bmpInfo.bmiHeader.biHeight < 0\n");
        return -1;
    }
    return 0;
}

HI_S32 LOTO_OSD_GetBmpInfo(const char*           filename,
                           OSD_BITMAPFILEHEADER* pBmpFileHeader,
                           OSD_BITMAPINFO*       pBmpInfo) {
    FILE* pFile;

    HI_U16 bfType;

    if (NULL == filename) {
        LOGE("OSD_LoadBMP: filename=NULL\n");
        return -1;
    }

    if ((pFile = fopen((char*)filename, "rb")) == NULL) {
        LOGE("Open file faild:%s!\n", filename);
        return -1;
    }

    (void)fread(&bfType, 1, sizeof(bfType), pFile);
    if (bfType != 0x4d42) {
        LOGE("not bitmap file\n");
        fclose(pFile);
        return -1;
    }

    (void)fread(pBmpFileHeader, 1, sizeof(OSD_BITMAPFILEHEADER), pFile);
    (void)fread(pBmpInfo, 1, sizeof(OSD_BITMAPINFO), pFile);
    fclose(pFile);

    LOTO_OSD_CheckBmpInfo(pBmpInfo);

    return 0;
}

HI_VOID LOTO_OSD_GetDeviceNum(HI_S16* deviceNum) {
    int i = 0;

    for (i = 0; i < 3; i++) {
        switch (g_device_num[i]) {
            case '0':
                deviceNum[i] = 0;
                break;
            case '1':
                deviceNum[i] = 1;
                break;
            case '2':
                deviceNum[i] = 2;
                break;
            case '3':
                deviceNum[i] = 3;
                break;
            case '4':
                deviceNum[i] = 4;
                break;
            case '5':
                deviceNum[i] = 5;
                break;
            case '6':
                deviceNum[i] = 6;
                break;
            case '7':
                deviceNum[i] = 7;
                break;
            case '8':
                deviceNum[i] = 8;
                break;
            case '9':
                deviceNum[i] = 9;
                break;
            default:
                LOGE("Get device number character [%d] error!\n", i);
        }
    }
}

HI_VOID LOTO_OSD_GetTimeNum(HI_S16* timeNum) {
    time_t     timep;
    struct tm* pLocalTime;
    time(&timep);

#if 1
    pLocalTime = localtime(&timep);
#else
    struct tm tmLoc;
    timep += global_get_cfg_apptime_zone_seconds();
    global_convert_time(timep, 0, &tmLoc);
    pLocalTime = &tmLoc;
#endif

    /* year */
    timeNum[0] = (1900 + pLocalTime->tm_year) / 1000;
    timeNum[1] = ((1900 + pLocalTime->tm_year) / 100) % 10;
    timeNum[2] = ((1900 + pLocalTime->tm_year) % 100) / 10;
    timeNum[3] = (1900 + pLocalTime->tm_year) % 10;

    /* dash */
    timeNum[4] = -1;

    /* month */
    timeNum[5] = (1 + pLocalTime->tm_mon) / 10;
    timeNum[6] = (1 + pLocalTime->tm_mon) % 10;

    /* dash */
    timeNum[7] = -1;

    /* day */
    timeNum[8] = (pLocalTime->tm_mday) / 10;
    timeNum[9] = (pLocalTime->tm_mday) % 10;

    /* hour */
    timeNum[10] = (pLocalTime->tm_hour) / 10;
    timeNum[11] = (pLocalTime->tm_hour) % 10;

    /* colon */
    timeNum[12] = -1;

    /* minute */
    timeNum[13] = (pLocalTime->tm_min) / 10;
    timeNum[14] = (pLocalTime->tm_min) % 10;

    /* colon */
    timeNum[15] = -1;

    /* second */
    timeNum[16] = (pLocalTime->tm_sec) / 10;
    timeNum[17] = (pLocalTime->tm_sec) % 10;
}

HI_S32 LOTO_OSD_OpenBmpFile(FILE** pFile, const char** fileName,
                            HI_S32 fileCount) {
    int i = 0;
    for (i = 0; i < fileCount; i++) {
        if ((*(pFile + i) = fopen(*(fileName + i), "rb")) == NULL) {
            LOGE("Open file faild:%s!\n", *(fileName + i));
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

HI_S32 LOTO_OSD_CloseBmpFile(FILE** pFile, HI_S32 fileCount) {
    int i = 0;

    for (i = 0; i < fileCount; i++) {
        if (HI_SUCCESS != fclose(*(pFile + i))) {
            return HI_FAILURE;
        };
    }
    return HI_SUCCESS;
}

HI_S32 LOTO_OSD_GetBmpBuffer() {
    HI_S32 s32Ret;

    HI_U16 Bpp;       // Bytes per Pixel
    HI_U16 bmpWidth;  // bitmap width
    HI_U16 bmpHeight; // bitmap height

    const char* fileName[] = {"./res/0.bmp", "./res/1.bmp",  "./res/2.bmp",
                              "./res/3.bmp", "./res/4.bmp",  "./res/5.bmp",
                              "./res/6.bmp", "./res/7.bmp",  "./res/8.bmp",
                              "./res/9.bmp", "./res/10.bmp", "./res/11.bmp"};

    HI_U8* bmpBufferTem;

    HI_U32 stride;

    FILE* pFile[FILE_COUNT];

    /* 获取BMP文件信息，检测BMP信息是否正确 */
    int f = 0;
    for (f = 0; f < FILE_COUNT; f++) {
        if (LOTO_OSD_GetBmpInfo(*(fileName + f), gs_bmpFileHeader + f,
                                gs_bmpInfo + f) < 0) {
            return -1;
        };
    }

    /* 打开BMP文件 */
    s32Ret = LOTO_OSD_OpenBmpFile(pFile, fileName, FILE_COUNT);
    if (s32Ret != 0) {
        LOGE("LOTO_OSD_OpenBmpFile error!\n");
    }

    for (f = 0; f < FILE_COUNT; f++) {
        Bpp       = (HI_U16)gs_bmpInfo[f].bmiHeader.biBitCount / 8;
        bmpWidth  = (HI_U16)gs_bmpInfo[f].bmiHeader.biWidth;
        bmpHeight = (HI_U16)((gs_bmpInfo[f].bmiHeader.biHeight > 0)
                                 ? gs_bmpInfo[f].bmiHeader.biHeight
                                 : (-gs_bmpInfo[f].bmiHeader.biHeight));

        stride = bmpWidth * Bpp;
        if (stride % 4) {
            stride = (stride & 0xfffc) + 4; // 按4补齐
        }

        // LOGE("stride * bmpHeight = %d\n", stride * bmpHeight);

        bmpBufferTem = (HI_U8*)malloc(stride * bmpHeight);
        if (NULL == bmpBufferTem) {
            LOGE("Not enough memory to allocate!\n");
            s32Ret = LOTO_OSD_CloseBmpFile(pFile, FILE_COUNT);
            if (s32Ret != 0) {
                LOGE("LOTO_OSD_CloseBmpFile error!\n");
            }
            return -1;
        }

        fseek(*(pFile + f), gs_bmpFileHeader[f].bfOffBits, 0);
        if (fread(bmpBufferTem, 1, bmpHeight * stride, *(pFile + f)) !=
            (bmpHeight * stride)) {
            LOGE("fread (%d*%d)error!\n", bmpHeight, stride);
            perror("fread:");
        }

        bmpBuffer[f] = bmpBufferTem;

        if (bmpBuffer[f] == NULL) {
            LOGE("Get NO.%d bmp buffer error!\n", f);
            return HI_FAILURE;
        }
        // LOGD("Get NO.%d bmp buffer success!\n", f);

        // free(bmpBufferTem);
        // bmpBufferTem = NULL;
    }

    /* 关闭BMP文件 */
    s32Ret = LOTO_OSD_CloseBmpFile(pFile, FILE_COUNT);
    if (s32Ret != 0) {
        LOGE("LOTO_OSD_CloseBmpFile error!\n");
    }

    return HI_SUCCESS;
}

HI_S32 LOTO_OSD_FreeBmpBuffer() {
    int i = 0;

    for (i = 0; i < FILE_COUNT; i++) {
        if (bmpBuffer[i] == NULL) {
            LOGE("bmp[%d] buffer is empty\n", i);
            return HI_FAILURE;
        } else {
            free(bmpBuffer[i]);
        }
    }

    return HI_SUCCESS;
}

/**
 * @brief 判断获取到的位图大小是否超出画布容纳范围
 *
 * @param pCanvas 画布指针
 * @param bmpWidth 位图宽度
 * @param bmpHeight 位图高度
 * @param stride 位图跨度
 * @return HI_VOID
 */
HI_S32 LOTO_OSD_CheckBmpSize(OSD_LOGO_T* pCanvas, HI_U32 bmpWidth,
                             HI_U32 bmpHeight, HI_U32 stride) {
    if (stride > pCanvas->stride) {
        LOGE("Bitmap's stride(%d) is bigger than canvas's stide(%d). "
             "Load bitmap error!\n",
             stride, pCanvas->stride);
        return -1;
    }

    if (bmpHeight > pCanvas->height) {
        LOGE("Bitmap's height(%d) is bigger than canvas's height(%d). "
             "Load bitmap error!\n",
             bmpHeight, pCanvas->height);
        return -1;
    }

    if (bmpWidth > pCanvas->width) {
        LOGE("Bitmap's width(%d) is bigger than canvas's width(%d). "
             "Load bitmap error!\n",
             bmpWidth, pCanvas->width);
        return -1;
    }
    return 0;
}

/**
 * @brief 加载BMP图
 *
 * @param handle 		画布句柄
 * @param pCanvas
 * @param enFmt 		像素格式
 * @return HI_S32
 */
HI_S32 LOTO_OSD_LoadBmp(RGN_HANDLE handle, OSD_LOGO_T* pCanvas, OSD_COLOR_FMT_E enFmt) {
    HI_S32 s32Ret = 0;
    HI_U16 row    = 0;
    HI_U16 column = 0;
    HI_S32 k      = 0;
    HI_S32 bmpNumMax = 0;
    HI_U16 strideSum = 0;

    HI_U32 bmpWidth;
    HI_U32 bmpHeight;
    HI_U16 Bpp; // Byte per pixel
    HI_U16 BppDst = 2;
    HI_U16 space  = BppDst * 13; // bpp * pixels

    OSD_BITMAPINFO bmpInfoTem;
    HI_U8*         pBitmapAddr; // bitmap address
    HI_U8*         pCanvasAddr; // canvas address
    HI_U16         stride;      // 加载的位图的一行像素的字节量
    HI_U16         strideDst;

    HI_U8* pSrcPixel;
    HI_U8* pDstPixel;

    HI_S16 deviceNum[3] = {0};
    HI_S16 timeNum[18]  = {0};

    pCanvasAddr = pCanvas->pRGBBuffer;
    if (pCanvasAddr == NULL) {
        LOGE("Get canvas address error!\n");
        return HI_FAILURE;
    }

    switch (handle) {
        case OSD_HANDLE_DEVICENUM: {
            LOTO_OSD_GetDeviceNum(deviceNum);
            bmpNumMax = 3;
            break;
        }

        case OSD_HANDLE_TIMESTAMP: {
            LOTO_OSD_GetTimeNum(timeNum);
            bmpNumMax = 18;
            break;
        }

        default:
            break;
    }

    for (k = 0; k < bmpNumMax; k++) {
        if (handle == OSD_HANDLE_DEVICENUM) {
            bmpInfoTem  = gs_bmpInfo[deviceNum[k]];
            pBitmapAddr = bmpBuffer[deviceNum[k]];

        } else if (handle == OSD_HANDLE_TIMESTAMP) {
            if (k == 4 || k == 7) {
                // bmpFileHeaderTem = bmpFileHeader[10];
                bmpInfoTem  = gs_bmpInfo[10];
                pBitmapAddr = bmpBuffer[10];

            } else if (k == 12 || k == 15) {
                // bmpFileHeaderTem = bmpFileHeader[11];
                bmpInfoTem  = gs_bmpInfo[11];
                pBitmapAddr = bmpBuffer[11];

            } else {
                if (timeNum[k] != -1) {
                    // bmpFileHeaderTem = bmpFileHeader[timeNum[k]];
                    bmpInfoTem  = gs_bmpInfo[timeNum[k]];
                    pBitmapAddr = bmpBuffer[timeNum[k]];
                } else {
                    LOGE("Get time infomation error!\n");
                    return HI_FAILURE;
                }
            }
        }

        if (pBitmapAddr == NULL) {
            LOGE("Get bitmap [%d] address error!\n", k);
            return HI_FAILURE;
        }

        /* bitmap infomation */
        Bpp = bmpInfoTem.bmiHeader.biBitCount / 8;
        // LOGD("Bpp = %d\n", Bpp);

        bmpWidth  = (HI_U16)bmpInfoTem.bmiHeader.biWidth;
        bmpHeight = (HI_U16)((bmpInfoTem.bmiHeader.biHeight > 0)
                                 ? bmpInfoTem.bmiHeader.biHeight
                                 : (-bmpInfoTem.bmiHeader.biHeight));
        // LOGD("bmpWidth:%d, bmpHeight:%d \n", bmpWidth, bmpHeight);

        /* stride 按 4 补齐 */
        stride = bmpWidth * Bpp;
        if (stride % 4) {
            stride = (stride & 0xfffc) + 4;
        }

        strideDst = bmpWidth * BppDst;
        strideSum += strideDst;

        // printf("strideSum: %d\n", strideSum);

        if (strideSum > pCanvas->stride) {
            LOGE("strideSum is wider!\n");
            return HI_FAILURE;
        }

        if (enFmt == OSD_COLOR_FMT_RGB8888) {
            /* pixel by pixel */
            /* for (row = 0; row < bmpHeight; row++) {
                for (column = 0; column < bmpWidth; column++) {
                    pSrcPixel = pBitmapAddr + ((bmpHeight - 1) - row) * stride +
            column * Bpp; pDstPixel = pCanvasAddr + row * pCanvas->stride +
            column * Bpp + k * space; if (k >= 10) { pDstPixel += Bpp * 16;
                    }
                    pDstPixel[0] = pSrcPixel[0]; // r
                    pDstPixel[1] = pSrcPixel[1]; // g
                    pDstPixel[2] = pSrcPixel[2]; // b
                    pDstPixel[3] = pSrcPixel[3]; // a
                }
            } */

            /* stride by stride */
            for (row = 0; row < bmpHeight; row++) {
                pSrcPixel = pBitmapAddr + ((bmpHeight - 1) - row) * stride;
                pDstPixel = pCanvasAddr + row * pCanvas->stride + k * space;
                memcpy(pDstPixel, pSrcPixel, stride);
            }

        } else if(enFmt == OSD_COLOR_FMT_RGB1555){
            for (row = 0; row < bmpHeight; row++) {
                for (column = 0; column < bmpWidth; column++) {
                    pSrcPixel = pBitmapAddr + ((bmpHeight - 1) - row) * stride + column * Bpp;
                    pDstPixel = pCanvasAddr + row * pCanvas->stride + column * BppDst + k * space;
                    if (k >= 10) {
                        pDstPixel += space - 8;
                    }

                    Pixel_1555 pixel;
                    pixel.blue  = (pSrcPixel[0] >> 3) & 0x1F;
                    pixel.green = (pSrcPixel[1] >> 3) & 0x1F;
                    pixel.red   = (pSrcPixel[2] >> 3) & 0x1F;
                    pixel.alpha = (pSrcPixel[3] > 0 ? 1 : 0) & 0x01;

                    memcpy(pDstPixel, &pixel, sizeof(Pixel_1555));
                }
            }
        } else {
            LOGE("The pixel format is not supported!\n");
        }
    }

    if (s32Ret != HI_SUCCESS) {
        LOGE("load bmp error!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/**
 * @brief 添加图层
 *
 * @param handle 		画布句柄
 * @param ptrSurface 	图层
 * @param pu8Virt 		画布虚拟地址
 * @param u32Width 		画布宽度
 * @param u32Height 	画布高度
 * @param u32Stride 	画布跨距
 * @return HI_S32
 */
HI_S32 LOTO_OSD_CreateSurfaceByCanvas(RGN_HANDLE     handle,
                                      OSD_SURFACE_S* ptrSurface, 
                                      HI_U8* pu8Virt,
                                      HI_U32 u32Width, 
                                      HI_U32 u32Height,
                                      HI_U32 u32Stride) {
    HI_S32 s32Ret;

    OSD_LOGO_T pCanvas;
    pCanvas.pRGBBuffer = pu8Virt; // 画布虚拟数据地址 pstBitmap->pData =
                                  // stCanvasInfo.u64VirtAddr;
    pCanvas.width  = u32Width;
    pCanvas.height = u32Height;
    pCanvas.stride = u32Stride;

    s32Ret = LOTO_OSD_LoadBmp(handle, &pCanvas, ptrSurface->enColorFmt);
    if (s32Ret != HI_SUCCESS) {
        LOGE("LOTO_OSD_LoadBmp error!\n");
        return HI_FAILURE;
    }

    ptrSurface->u16Height = u32Height;
    ptrSurface->u16Width  = u32Width;
    ptrSurface->u16Stride = u32Stride;

    return HI_SUCCESS;
}

/**
 * @brief 更新画布信息
 *
 * @param handle 		画布句柄
 * @param pstBitmap 	位图指针，保存位图数据的地址为画布的虚拟地址
 * @param pstSize 		画布大小
 * @param u32Stride 	画布跨距
 * @param enPixelFmt 	像素格式
 * @return HI_S32 		0: success; other: error codes
 */
HI_S32 LOTO_OSD_UpdateCanvasInfo(RGN_HANDLE handle, BITMAP_S* pstBitmap,
                                 SIZE_S* pstSize, HI_U32 u32Stride,
                                 PIXEL_FORMAT_E enPixelFmt) {
    OSD_SURFACE_S Surface;

    if (PIXEL_FORMAT_RGB_1555 == enPixelFmt) {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;

    } else if (PIXEL_FORMAT_RGB_4444 == enPixelFmt) {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB4444;

    } else if (PIXEL_FORMAT_RGB_8888 == enPixelFmt) {
        Surface.enColorFmt = OSD_COLOR_FMT_RGB8888;

    } else {
        LOGE("Pixel format is not support!\n");
        return HI_FAILURE;
    }

    LOTO_OSD_CreateSurfaceByCanvas(handle, &Surface, (HI_U8*)(pstBitmap->pData),
                                   pstSize->u32Width, pstSize->u32Height,
                                   u32Stride);

    pstBitmap->u32Width  = Surface.u16Width;
    pstBitmap->u32Height = Surface.u16Height;

    if (PIXEL_FORMAT_RGB_1555 == enPixelFmt) {
        pstBitmap->enPixelFormat = PIXEL_FORMAT_RGB_1555;

    } else if (PIXEL_FORMAT_RGB_4444 == enPixelFmt) {
        pstBitmap->enPixelFormat = PIXEL_FORMAT_RGB_4444;

    } else if (PIXEL_FORMAT_RGB_8888 == enPixelFmt) {
        pstBitmap->enPixelFormat = PIXEL_FORMAT_RGB_8888;
    }

    return HI_SUCCESS;
}

HI_S32 LOTO_OSD_AddBitMap(RGN_HANDLE handle) {
    HI_S32     s32Ret = HI_SUCCESS;
    RGN_ATTR_S stRgnAttr;
    BITMAP_S   stBitmap;
    SIZE_S     stSize;
    HI_S32     bpp; // bytes per pixel

    s32Ret = HI_MPI_RGN_GetAttr(handle, &stRgnAttr);
    if (HI_SUCCESS != s32Ret) {
        LOGE("HI_MPI_RGN_GetAttr failed with %#x\n", s32Ret);
        return s32Ret;
    }

    stSize.u32Width  = stRgnAttr.unAttr.stOverlayEx.stSize.u32Width;
    stSize.u32Height = stRgnAttr.unAttr.stOverlayEx.stSize.u32Height;

    // printf("Width:  %u\n"
    //        "Height: %u\n",
    //        stSize.u32Width, stSize.u32Height);

switch (stRgnAttr.unAttr.stOverlay.enPixelFmt) {
    case PIXEL_FORMAT_RGB_2BPP: 
    case PIXEL_FORMAT_RGB_4444:
    case PIXEL_FORMAT_RGB_1555:
        bpp = 2;
        break;
    
    case PIXEL_FORMAT_RGB_8BPP: 
    case PIXEL_FORMAT_RGB_8888:
        bpp = 8;
        break;

    default:
        bpp = 2;
        break;
}

    stBitmap.pData = (void*)malloc(stSize.u32Width * stSize.u32Height * bpp);
    if (NULL == stBitmap.pData) {
        LOGE("malloc osd memroy err!\n");
        return HI_FAILURE;
    }

    memset(stBitmap.pData, 0, stSize.u32Width * stSize.u32Height * bpp);

    HI_U32 u32Stride = stSize.u32Width * bpp;
    if (u32Stride % 4) {
        u32Stride = (u32Stride & 0xfffc) + 4;
    }

    s32Ret = LOTO_OSD_UpdateCanvasInfo(handle, &stBitmap, &stSize, u32Stride,
                                       stRgnAttr.unAttr.stOverlay.enPixelFmt);
    if (HI_SUCCESS != s32Ret) {
        LOGE("LOTO_OSD_UpdateCanvasInfo failed with %#x.\n", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_RGN_SetBitMap(handle, &stBitmap);
    if (s32Ret != HI_SUCCESS) {
        LOGE("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    if (NULL != stBitmap.pData) {
        free(stBitmap.pData);
        stBitmap.pData = NULL;
    }

    return HI_SUCCESS;
}

HI_VOID* LOTO_OSD_AddVideoOsd(void* handle) {
    HI_S32     s32Ret = HI_SUCCESS;
    time_t     timep;
    struct tm* pLocalTime;
    HI_S32     seconds = 80;

    OsdHanlde* handle_info = (OsdHanlde*)handle;

    if (handle == NULL) {
        LOGE("OSD handle is empty! \n");
    }

    RGN_HANDLE deviceNumHandle = handle_info->deviceNum;
    RGN_HANDLE timestampHandle = handle_info->timestamp;
    free(handle_info);

    do {
        /* Device Num */
        s32Ret = LOTO_OSD_AddBitMap(deviceNumHandle);
        if (s32Ret != HI_SUCCESS) {
            LOGE("LOTO_OSD_AddBitMap for DeviceNum failed!\n");
            break;
        }

        /* Timestamp */
        while (1) {
            time(&timep);
            pLocalTime = localtime(&timep);
            if (seconds == pLocalTime->tm_sec) {
                usleep(100 * 1000);
                continue;
            }

            seconds = pLocalTime->tm_sec;
            // LOGD("Second = %d\n", seconds);

            // LOGD("LOTO_OSD_AddBitMap start: %s\n", GetTimestampString());
            s32Ret = LOTO_OSD_AddBitMap(timestampHandle);
            if (s32Ret != HI_SUCCESS) {
                LOGE("LOTO_OSD_AddBitMap for Timestamp failed!\n");
                break;
            }
            // LOGD("LOTO_OSD_AddBitMap end: %s\n", GetTimestampString());
        }
    } while (0);

    LOTO_OSD_FreeBmpBuffer();

    pthread_detach(pthread_self());

    return 0;
}

HI_S32 LOTO_OSD_CreateVideoOsdThread(HI_VOID) {
    HI_S32 s32Ret;

    OsdHanlde* handle_info = (OsdHanlde*)malloc(sizeof(OsdHanlde));
    handle_info->deviceNum = OSD_HANDLE_DEVICENUM;
    handle_info->timestamp = OSD_HANDLE_TIMESTAMP;

    OsdRegionInfo deviceNumRgnInfo = {
        .width  = 2 * 8 * 3,
        .height = 2 * 11,
        .pointX = 16 * 2,
        .pointY = 2 * 5,
    };

    OsdRegionInfo timestampRgnInfo = {
        .width  = 2 * 8 * 19,
        .height = 2 * 11,
        .pointX = 16 * 5,
        .pointY = 2 * 5,
    };

    /* Create DeviceNum Region */
    s32Ret = LOTO_OSD_REGION_Create(handle_info->deviceNum, deviceNumRgnInfo.width,
                               deviceNumRgnInfo.height);
    if (s32Ret != HI_SUCCESS) {
        LOGE("DeviceNum Region: LOTO_OSD_REGION_Create failed\n");
        return HI_FAILURE;
    }

    s32Ret = LOTO_OSD_REGION_AttachToChn(handle_info->deviceNum,
                                         deviceNumRgnInfo.pointX,
                                         deviceNumRgnInfo.pointY);
    if (s32Ret != HI_SUCCESS) {
        LOGE("DeviceNum Region: LOTO_OSD_REGION_AttachToChn failed\n");
        return HI_FAILURE;
    }

    /*  Create Timestamp Region */
    s32Ret = LOTO_OSD_REGION_Create(handle_info->timestamp, timestampRgnInfo.width,
                               timestampRgnInfo.height);
    if (s32Ret != HI_SUCCESS) {
        LOGE("Timestamp Region: LOTO_OSD_REGION_Create failed\n");
        return HI_FAILURE;
    }

    s32Ret = LOTO_OSD_REGION_AttachToChn(handle_info->timestamp,
                                         timestampRgnInfo.pointX,
                                         timestampRgnInfo.pointY);
    if (s32Ret != HI_SUCCESS) {
        LOGE("Timestamp Region: LOTO_OSD_REGION_AttachToChn failed\n");
        return HI_FAILURE;
    }

    s32Ret = LOTO_OSD_GetBmpBuffer();
    if (s32Ret != HI_SUCCESS) {
        LOGE("LOTO_OSD_GetBmpBuffer error!\n");
    }

    pthread_t osd_thread_id = 0;
    pthread_create(&osd_thread_id, NULL, LOTO_OSD_AddVideoOsd, handle_info);

    return HI_SUCCESS;
}
