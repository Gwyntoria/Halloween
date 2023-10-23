#include "xiecc_rtmp.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "faac.h"
#include <stdbool.h>

#define AAC_ADTS_HEADER_SIZE 7
static uint32_t find_start_code(uint8_t *buf, uint32_t zeros_in_startcode)   
{   
  uint32_t info;   
  uint32_t i;   
   
  info = 1;   
  if ((info = (buf[zeros_in_startcode] != 1)? 0: 1) == 0)   
      return 0;   
       
  for (i = 0; i < zeros_in_startcode; i++)   
    if (buf[i] != 0)   
    { 
        info = 0;
        break;
    };   
     
  return info;   
}   

uint8_t * get_nal(uint32_t *len, uint8_t **offset, uint8_t *start, uint32_t total)
{
    uint32_t info;
    uint8_t *q ;
    uint8_t *p  =  *offset;
    *len = 0;

    while(1) {
        info =  find_start_code(p, 3);
        if (info == 1)
            break;
        p++;
        if ((p - start) >= total)
            return NULL;
    }
    q = p + 4;
    p = q;
    while(1) {
        info =  find_start_code(p, 3);
        if (info == 1)
            break;
        p++;
        if ((p - start) >= total)
            return NULL;
    }
    
    *len = (p - q);
    *offset = p;
    return q;
}
uint8_t *get_adts(uint32_t *len, uint8_t **offset, uint8_t *start, uint32_t total)
{
    uint8_t *p  =  *offset;
    uint32_t frame_len_1;
    uint32_t frame_len_2;
    uint32_t frame_len_3;
    uint32_t frame_length;
   
    if (total < AAC_ADTS_HEADER_SIZE) {
        return NULL;
    }
    if ((p - start) >= total) {
        return NULL;
    }
    
    if (p[0] != 0xff) {
        return NULL;
    }
    if ((p[1] & 0xf0) != 0xf0) {
        return NULL;
    }
    frame_len_1 = p[3] & 0x03;
    frame_len_2 = p[4];
    frame_len_3 = (p[5] & 0xe0) >> 5;
    frame_length = (frame_len_1 << 11) | (frame_len_2 << 3) | frame_len_3;
    *offset = p + frame_length;
    *len = frame_length;
    return p;
}


typedef unsigned long ULONG;
typedef unsigned int UINT;
typedef unsigned char BYTE;
int main(int argc, char *argv[])
{
	char serverStrBuf[100];
	sprintf(serverStrBuf, "rtmp://%s:%s/%s/livestream_chn_%s",argv[1]/*server*/,argv[2]/*port*/,argv[3],/*myapp*/argv[4]/*channel*/);
	printf("serverStrBuf is %s\n",serverStrBuf);
	char vpipe[15];
	char apipe[15];
	sprintf(vpipe,"/tmp/vfifoChn%s",argv[4]);	
	sprintf(apipe,"/tmp/afifoChn%s",argv[4]);
	
	printf("vpipe is %s\n",vpipe);	
	printf("apipe is %s\n",apipe);
    void*prtmp = rtmp_sender_alloc(serverStrBuf/*"rtmp://192.168.2.102:1936/myapp/livestream_chn_1"*/); //return handle
    rtmp_sender_start_publish(prtmp, 0, 0);
    uint8_t *audio_buf_offset ;
    uint32_t audio_len;
    uint8_t *p_audio;
    int ret = 0;
    int vpipeFD = open(vpipe/*"/tmp/vfifoChn1"*/, O_RDONLY|O_NONBLOCK);
	if (vpipeFD < 0)
	{
		printf("open FOR READ failed\n");
	}
    int apipeFD = open(apipe/*"/tmp/afifoChn1"*/, O_RDONLY|O_NONBLOCK);
	if (apipeFD < 0)
	{
		printf("open FOR aREAD failed\n");
	}

    ULONG nSampleRate = 8000;  // 采样率
    UINT nChannels = 1;         // 声道数
    UINT nBit = 16;             // 单样本位数
    ULONG nInputSamples = 0;	//输入样本数
    ULONG nMaxOutputBytes = 0;	//输出所需最大空间
	ULONG nMaxInputBytes=0;     //输入最大字节
    faacEncHandle hEncoder;		//aac句柄
    faacEncConfigurationPtr pConfiguration;//aac设置指针 
    char* abuffer;
	char* vbuffer;

    BYTE* pbAACBuffer;
  // (1) Open FAAC engine
    hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);//初始化aac句柄，同时获取最大输入样本，及编码所需最小字节
	nMaxInputBytes=nInputSamples*nBit/8;//计算最大输入字节,跟据最大输入样本数
	printf("nInputSamples:%d nMaxInputBytes:%d nMaxOutputBytes:%d\n", nInputSamples, nMaxInputBytes,nMaxOutputBytes);
    if(hEncoder == NULL)
    {
        printf("[ERROR] Failed to call faacEncOpen()\n");
        return -1;
    }
    pbAACBuffer = (char*)malloc(nMaxOutputBytes);
    // (2.1) Get current encoding configuration
    pConfiguration = faacEncGetCurrentConfiguration(hEncoder);//获取配置结构指针
    pConfiguration->inputFormat = FAAC_INPUT_16BIT;
	pConfiguration->outputFormat=1;
	pConfiguration->useTns=true;
	pConfiguration->useLfe=false;
	pConfiguration->aacObjectType=LOW;
	pConfiguration->mpegVersion = MPEG4; 
	pConfiguration->shortctl=SHORTCTL_NORMAL;
	pConfiguration->quantqual=60;
	pConfiguration->bandWidth=80000;
	pConfiguration->bitRate=0;
	printf("!!!!!!!!!!!!!pConfiguration->outputFormat is %d\n",pConfiguration->outputFormat);
    // (2.2) Set encoding configuration
    ret = faacEncSetConfiguration(hEncoder, pConfiguration);//设置配置，根据不同设置，耗时不一样

	abuffer = (char*)malloc(nMaxInputBytes);
	if(abuffer == NULL)
		exit(0);
	vbuffer = (char*)malloc(25*1024);
	if(vbuffer == NULL)
		exit(0);	
	char* output = (char*)malloc(10*1024);  
	static unsigned int audioCount = 0;
	uint32_t start_time = RTMP_GetTime();
	uint32_t timeCount = 0;
	timeCount = 0;//RTMP_GetTime();
    while (1) {
	//	timeCount += 0.01;
	//	start_time += timeCount;
		ret = read(vpipeFD, vbuffer, 25 * 1024);
		if(ret > 0){     
			rtmp_sender_write_video_frame(prtmp, vbuffer, ret, timeCount, 0,start_time);
			timeCount += 2;
		}
		ret = read(apipeFD, abuffer, nMaxInputBytes - audioCount);
		if(ret >0){
			if(audioCount < 1*2048)	{
				memcpy(output + audioCount,abuffer,ret);
				audioCount += ret;
				ret = 0;
				if(audioCount >= 1*2048){
					ret = audioCount;
					audioCount = 0;
				}
			}else{
				ret = audioCount;
				audioCount = 0;
			}
		}
		if(ret > 0){
			nInputSamples = ret/ (nBit / 8);
			// (3) Encode
			ret = faacEncEncode(hEncoder, (int*) output, nInputSamples, pbAACBuffer, nMaxOutputBytes);
			audio_buf_offset = pbAACBuffer;
			if (ret > 0){
				p_audio = get_adts(&audio_len, &audio_buf_offset, pbAACBuffer, ret);
				if (p_audio == NULL){
					printf("p_audio is null,this should not happen!!!!\n");
					continue;
				}
				rtmp_sender_write_audio_frame(prtmp, p_audio, audio_len, timeCount,start_time);
			}			
		}
	//	timeCount += 200;/*15 perfect*/
		//printf("!!");
		usleep(1);
    }
}
