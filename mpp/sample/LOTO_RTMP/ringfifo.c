/*ringbuf .c*/

#include<stdio.h>
#include<ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ringfifo.h"
// #include "rtputils.h"
#include "sample_comm.h"
#include <inttypes.h>
#include "common.h"
#include "EasyAACEncoderAPI.h"
extern Easy_Handle g_Easy_H;


#define NMAX 32

int iput = 0; /* ���λ������ĵ�ǰ����λ�� */
int iget = 0; /* �������ĵ�ǰȡ��λ�� */
int n = 0; /* ���λ������е�Ԫ�������� */

int v_count = 0;
int v_n_count = 0;

int a_iput = 0;
int a_iget = 0;
int a_n = 0;

struct ringbuf ringfifo[NMAX];
struct ringbuf a_ringfifo[NMAX];
unsigned char AACBuffer[1024];
unsigned char PCMBuffer[1024*5];
int PCMLen = 0, AACLen = 0;

extern int UpdateSpsOrPps(unsigned char *data,int frame_type,int len);

enum H264_FRAME_TYPE {FRAME_TYPE_I, FRAME_TYPE_P, FRAME_TYPE_B};

/* ���λ������ĵ�ַ��ż��㺯����������﻽�ѻ�������β�������ƻص�ͷ����
���λ���������Ч��ַ���Ϊ��0��(NMAX-1)
*/
void ringmalloc(int size)
{
    int i;
    for(i =0; i<NMAX; i++)
    {
        ringfifo[i].buffer = malloc(size);
        ringfifo[i].size = 0;
        ringfifo[i].frame_type = 0;
       // printf("FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
    }
    iput = 0; /* ���λ������ĵ�ǰ����λ�� */
    iget = 0; /* �������ĵ�ǰȡ��λ�� */
    n = 0; /* ���λ������е�Ԫ�������� */
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
void ringreset()
{
    iput = 0; /* ���λ������ĵ�ǰ����λ�� */
    iget = 0; /* �������ĵ�ǰȡ��λ�� */
    n = 0; /* ���λ������е�Ԫ�������� */
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
void ringfree(void)
{
    int i;
    printf("begin free mem\n");
    for(i =0; i<NMAX; i++)
    {
       // printf("FREE FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
        free(ringfifo[i].buffer);
        ringfifo[i].size = 0;
    }
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
int addring(int i)
{
    return (i+1) == NMAX ? 0 : i+1;
}

/**************************************************************************************************
**
**
**
**************************************************************************************************/
/* �ӻ��λ�������ȡһ��Ԫ�� */

int ringget(struct ringbuf *getinfo)
{
    int Pos;
    if(n>0)
    {
        // int is_delay = 0;//(n > 3);
        // if (is_delay)
        //     printf("Enter ringget video n = %d\n", n);

        Pos = iget;
        iget = addring(iget);
        n--;

        // while (n > 0)
        // {
        //     if (ringfifo[Pos].frame_type == FRAME_TYPE_I)
        //         break;

        //     Pos = iget;
        //     iget = addring(iget);
        //     n--;
        // }
        
        getinfo->buffer = (ringfifo[Pos].buffer);
        getinfo->frame_type = ringfifo[Pos].frame_type;
        getinfo->size = ringfifo[Pos].size;
        getinfo->timestamp = ringfifo[Pos].timestamp;
        getinfo->getframe_timestamp = ringfifo[Pos].getframe_timestamp;
        // printf("Get FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",Pos,getinfo->size,(int)(getinfo->buffer),getinfo->frame_type);

        // if (is_delay)
        // {
        //     LOGD("[%s] Exit ringget video n = %d \n ", log_Time(), n);
        // }

        // if (n > 5)
        //     v_n_count ++;

        // if (v_count == 0 || v_count == 3000)
        // {
        //     LOGD("[%s] ringget:  video timestamp = %"PRIu64", over 5 buffers = %d", log_Time(), getinfo->timestamp / 1000, v_n_count);
        //     v_count = 0;
        //     v_n_count = 0;
        // }
        // v_count ++;

        return ringfifo[Pos].size;
    }
    else
    {
        //printf("Buffer is empty\n");
        return 0;
    }
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
/* ���λ������з���һ��Ԫ��*/
void ringput(unsigned char *buffer,int size,int encode_type)
{

    if(n<NMAX)
    {
        memcpy(ringfifo[iput].buffer,buffer,size);
        ringfifo[iput].size= size;
        ringfifo[iput].frame_type = encode_type;
        //printf("Put FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",iput,ringfifo[iput].size,(int)(ringfifo[iput].buffer),ringfifo[iput].frame_type);
        iput = addring(iput);
        n++;
    }
    else
    {
        //  printf("Buffer is full\n");
    }
}

/**************************************************************************************************
**
**
**
**************************************************************************************************/
HI_S32 HisiPutH264DataToBuffer(VENC_STREAM_S *pstStream)
{
	HI_S32 i,j;
	HI_S32 len=0,off=0,len2=2;
	unsigned char *pstr;
	int iframe=0;

    // printf("HisiPutH264DataToBuffer: count = %d \n ", pstStream->u32PackCount);

	for (i = 0; i < pstStream->u32PackCount; i++)
	{
		len += pstStream->pstPack[i].u32Len[0];
        if (pstStream->pstPack[i].u32Len[1] > 0)
        {
            len += pstStream->pstPack[i].u32Len[1];
        }
	}

    // printf("HisiPutH264DataToBuffer: length = %d \n ", len);

    if(n<NMAX)
    {
        ringfifo[iput].getframe_timestamp = get_timestamp(NULL, 1);

		for (i = 0; i < pstStream->u32PackCount; i++)
		{
			memcpy(ringfifo[iput].buffer+off, pstStream->pstPack[i].pu8Addr[0], pstStream->pstPack[i].u32Len[0]);
			off += pstStream->pstPack[i].u32Len[0];
			pstr = pstStream->pstPack[i].pu8Addr[0];
			if(pstr[4]==0x67)
			{
				iframe=1;
			}

            if (pstStream->pstPack[i].u32Len[1] > 0)
            {
                memcpy(ringfifo[iput].buffer+off, pstStream->pstPack[i].pu8Addr[1], pstStream->pstPack[i].u32Len[1]);
                off += pstStream->pstPack[i].u32Len[1];
                pstr = pstStream->pstPack[i].pu8Addr[1];
                if(pstr[4]==0x67)
                {
                    iframe=1;
                }
            }
		}

        ringfifo[iput].size= len;
        ringfifo[iput].timestamp = pstStream->pstPack[0].u64PTS;

        // printf("HisiPutH264DataToBuffer: ringfifo[iput].timestamp = %"PRIu64" \n ", ringfifo[iput].timestamp);
		// printf("iframe=%d\n",iframe);
		if(iframe)
		{
			ringfifo[iput].frame_type = FRAME_TYPE_I;
		}
        	
		else
			ringfifo[iput].frame_type = FRAME_TYPE_P;
        iput = addring(iput);
        n++;
    }
	 return HI_SUCCESS;
}

int get_video_count()
{
    return  n;
}


/* for Audio
*
*
*/

void ringmalloc_audio(int size)
{
    int i;
    for(i =0; i<NMAX; i++)
    {
        a_ringfifo[i].buffer = malloc(size);
        a_ringfifo[i].size = 0;
        a_ringfifo[i].frame_type = 0;
       // printf("FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
    }
    a_iput = 0; /* ���λ������ĵ�ǰ����λ�� */
    a_iget = 0; /* �������ĵ�ǰȡ��λ�� */
    a_n = 0; /* ���λ������е�Ԫ�������� */
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
void ringreset_audio()
{
    a_iput = 0; /* ���λ������ĵ�ǰ����λ�� */
    a_iget = 0; /* �������ĵ�ǰȡ��λ�� */
    a_n = 0; /* ���λ������е�Ԫ�������� */
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
void ringfree_audio(void)
{
    int i;
    printf("begin free mem\n");
    for(i =0; i<NMAX; i++)
    {
       // printf("FREE FIFO INFO:idx:%d,len:%d,ptr:%x\n",i,ringfifo[i].size,(int)(ringfifo[i].buffer));
        free(a_ringfifo[i].buffer);
        a_ringfifo[i].size = 0;
    }
}

/**************************************************************************************************
**
**
**
**************************************************************************************************/
/* �ӻ��λ�������ȡһ��Ԫ�� */

// int ringget_audio(struct ringbuf *getinfo)
// {
//     int Pos;
//     unsigned int out_len = 0;
//     int i = 0;

//     if(a_n>0)
//     {
//         Pos = a_iget;
//         a_iget = addring(a_iget);
//         a_n--;

//         // unsigned char tmp[2048*5] = "";
//         // for (i = 0; i < (a_ringfifo[Pos].size/2); i ++)
//         // {
//         //     sprintf(tmp, "%s 0x%2X", tmp, a_ringfifo[Pos].buffer[i]);
//         // }
//         // LOGD("[%s] ringget_audio: data = %s \n ", log_Time(), tmp);

//         int res = Easy_AACEncoder_Encode(g_Easy_H, a_ringfifo[Pos].buffer, a_ringfifo[Pos].size, AACBuffer, &out_len);
//         // LOGD("[%s] ringget_audio: res = %d, out_len = %d \n ", log_Time(), res, out_len);

//         if(res > 0)
//         {
//             // if (PCMLen > 0)
//             // {

//             // }
//             // res = Easy_AACEncoder_Encode(g_Easy_H, a_ringfifo[Pos].buffer, a_ringfifo[Pos].size, AACBuffer, &out_len);

//             getinfo->buffer = AACBuffer;
//             getinfo->size = out_len;
//             AACLen = out_len;
//             getinfo->timestamp = a_ringfifo[Pos].timestamp;
//             getinfo->getframe_timestamp = a_ringfifo[Pos].getframe_timestamp;
//             // LOGD("[%s] ringget_audio: u64TimeStamp = %"PRIu64", Pos = %d \n ", log_Time(), getinfo->timestamp, Pos);
//             //printf("Get FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",Pos,getinfo->size,(int)(getinfo->buffer),getinfo->frame_type);
//             return out_len;
//         }
//         else if (AACLen > 0)
//         {
//             getinfo->buffer = AACBuffer;
//             getinfo->size = AACLen;
//             getinfo->timestamp = a_ringfifo[Pos].timestamp;
//             getinfo->getframe_timestamp = a_ringfifo[Pos].getframe_timestamp;
//             // LOGD("[%s] ringget_audio: u64TimeStamp = %"PRIu64", Pos = %d \n ", log_Time(), getinfo->timestamp, Pos);
//             return AACLen;
//         }
//         else
//         {
//             return  0;
//         }
//     }
//     else
//     {
//         //printf("Buffer is empty\n");
//         return 0;
//     }
// }

int ringget_audio(struct ringbuf *getinfo)
{
    int Pos;
    if(a_n>0)
    {
        Pos = a_iget;
        a_iget = addring(a_iget);
        a_n--;
        getinfo->buffer = (a_ringfifo[Pos].buffer);
        getinfo->size = a_ringfifo[Pos].size;
        getinfo->timestamp = a_ringfifo[Pos].timestamp;
        getinfo->getframe_timestamp = a_ringfifo[Pos].getframe_timestamp;
        //printf("Get FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",Pos,getinfo->size,(int)(getinfo->buffer),getinfo->frame_type);
        return a_ringfifo[Pos].size;
    }
    else
    {
        //printf("Buffer is empty\n");
        return 0;
    }
}
/**************************************************************************************************
**
**
**
**************************************************************************************************/
/* ���λ������з���һ��Ԫ��*/
void ringput_audio(unsigned char *buffer,int size)
{
    if(a_n<NMAX)
    {
        memcpy(a_ringfifo[a_iput].buffer,buffer,size);
        a_ringfifo[a_iput].size= size;
        //printf("Put FIFO INFO:idx:%d,len:%d,ptr:%x,type:%d\n",iput,ringfifo[iput].size,(int)(ringfifo[iput].buffer),ringfifo[iput].frame_type);
        a_iput = addring(a_iput);
        a_n++;
    }
    else
    {
        //  printf("Buffer is full\n");
    }
}

/**************************************************************************************************
**
**
**
**************************************************************************************************/
// HI_S32 HisiPutAACDataToBuffer(AUDIO_STREAM_S *aacStream)
// {
// 	HI_S32 i,j;

//     if(a_n<NMAX)
//     {
//         a_ringfifo[a_iput].getframe_timestamp = get_timestamp(NULL, 1);
// 		memcpy(a_ringfifo[a_iput].buffer, aacStream->pStream, aacStream->u32Len);
//         a_ringfifo[a_iput].size= aacStream->u32Len;
//         a_ringfifo[a_iput].timestamp = aacStream->u64TimeStamp;

//         // printf("HisiPutAACDataToBuffer: aacStream->u64TimeStamp = %"PRIu64" \n ", aacStream->u64TimeStamp);

//         //printf("Put FIFO INFO:idx:%d,len:%d,ptr:%x\n",a_iput,aacStream->u32Len,(int)(aacStream->pStream));

//         a_iput = addring(a_iput);
//         a_n++;
//     }
// 	 return HI_SUCCESS;
// }

HI_S32 HisiPutAACDataToBuffer(unsigned char *pAACData, unsigned int iDataLen, unsigned long long u64TimeStamp, int iHead)
{
    if(a_n<NMAX)
    {
        // if (iHead == 1)
        // {
        //     a_ringfifo[a_iput].getframe_timestamp = get_timestamp(NULL, 1);
        //     memcpy(a_ringfifo[a_iput].buffer, pAACData, iDataLen);
        //     a_ringfifo[a_iput].size= iDataLen;
        //     a_ringfifo[a_iput].timestamp = u64TimeStamp;
        //     LOGD("[%s] HisiPutAACDataToBuffer: u64TimeStamp = %"PRIu64", pos = %d, a_ringfifo[a_iput].size = %d \n ", log_Time(), u64TimeStamp, a_iput, a_ringfifo[a_iput].size);
        // }
        // else
        // {
        //     a_ringfifo[a_iput].getframe_timestamp = get_timestamp(NULL, 1);
        //     if (a_iput == 0)
        //     {
        //         memcpy(a_ringfifo[a_iput].buffer, a_ringfifo[NMAX-1].buffer, iDataLen);
        //     }
        //     else
        //     {
        //         memcpy(a_ringfifo[a_iput].buffer, a_ringfifo[a_iput-1].buffer, iDataLen);
        //     }
            
        //     a_ringfifo[a_iput].size = iDataLen;
        //     a_ringfifo[a_iput].timestamp = u64TimeStamp;

        //     LOGD("[%s] HisiPutAACDataToBuffer: u64TimeStamp = %"PRIu64", pos = %d, a_ringfifo[a_iput].size = %d \n ", log_Time(), u64TimeStamp, a_iput, a_ringfifo[a_iput].size);
        // }

        a_ringfifo[a_iput].getframe_timestamp = get_timestamp(NULL, 1);
        memcpy(a_ringfifo[a_iput].buffer, pAACData, iDataLen);
        a_ringfifo[a_iput].size= iDataLen;
        a_ringfifo[a_iput].timestamp = u64TimeStamp;
        // LOGD("[%s] HisiPutAACDataToBuffer: u64TimeStamp = %"PRIu64", pos = %d, a_ringfifo[a_iput].size = %d \n ", log_Time(), u64TimeStamp, a_iput, a_ringfifo[a_iput].size);

        a_iput = addring(a_iput);
        a_n++;
    }
	 return HI_SUCCESS;
}