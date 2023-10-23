/*
 * Common.c:
 *
 * By Jessica Mao 2020/05/18
 *
 * Copyright (c) 2012-2020 Lotogram Inc. <lotogram.com, zhuagewawa.com>

 * Version 1.0.0.73	Details in update.log
 ***********************************************************************
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <time.h>
#include "common.h"
#include "md5.h"

 /*
 * log_Time:
 *  Return the formated time
 *********************************************************************************
 */
char *log_Time(void)
{
    struct  tm      *ptm;
    struct  timeb   stTimeb;
    static  char    szTime[256] = {0};

    ftime(&stTimeb);
    ptm = localtime(&stTimeb.time);
    sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d.%03d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, stTimeb.millitm);
    //szTime[23] = 0;
    return szTime;
}

int get_mac(char* mac)
{
    int sockfd;
    struct ifreq tmp;
    char mac_addr[30];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0)
    {
        perror("create socket fail\n");
        return -1;
    }

    memset(&tmp,0,sizeof(struct ifreq));
    strncpy(tmp.ifr_name,"eth0",sizeof(tmp.ifr_name)-1);
    if( (ioctl(sockfd,SIOCGIFHWADDR,&tmp)) < 0 )
    {
        printf("mac ioctl error\n");
        return -1;
    }

    sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x",
            (unsigned char)tmp.ifr_hwaddr.sa_data[0],
            (unsigned char)tmp.ifr_hwaddr.sa_data[1],
            (unsigned char)tmp.ifr_hwaddr.sa_data[2],
            (unsigned char)tmp.ifr_hwaddr.sa_data[3],
            (unsigned char)tmp.ifr_hwaddr.sa_data[4],
            (unsigned char)tmp.ifr_hwaddr.sa_data[5]
            );
    printf("local mac:%s\n", mac_addr);
    close(sockfd);
    memcpy(mac,mac_addr,strlen(mac_addr));

    return 0;
}

long long string2int(const char *str)
{
	char flag = '+';//指示结果是否带符号
	long long  res = 0;

	if(*str=='-')//字符串带负号
	{
		++str;//指向下一个字符
		flag = '-';//将标志设为负号
	}
	//逐个字符转换，并累加到结果res
	while(*str>=48 && *str<=57)//如果是数字才进行转换，数字0~9的ASCII码：48~57
	{
		res = 10*res+  *str++-48;//字符'0'的ASCII码为48,48-48=0刚好转化为数字0
	}

    if(flag == '-')//处理是负数的情况
	{
		res = -res;
	}

	return res;
}

int string_reverse(char * strSrc)
{
    int len = 0;
    int i = 0;
    char * output = NULL;
    char * pstr = strSrc;
    while(* pstr)
    {
        pstr++;
        len++;
    }
    output = (char *)malloc(len);
    if(output == NULL)
    {
        perror("malloc");
        return -1;
    }
    for(i = 0; i < len ;i++)
    {
        output[i] = strSrc[len - i -1];
        //printf("output[%d] = %c\n",len - i -1,strSrc[len - i - 1]);
    }
    output[len] = '\0';
    strcpy(strSrc, output);
    free(output);
    return 0;
}

int  int2string(long long value, char * output)
{
    int index = 0;
    if(value == 0)
    {
        output[0] = value + '0';
        return 1;
    }
    else
    {
        while(value)
        {
            output[index] = value % 10 + '0';
            index ++;
            value /= 10;
        }
        string_reverse(output);
        return 1;
    }
}

long long get_timestamp(char* pszTS, int isMSec)
{
    long long 	timestamp;
    char        szT[64] = "";
    struct 	timeval tv ;
    gettimeofday(&tv,NULL);
    if (isMSec > 0)
        timestamp = (long long)tv.tv_sec*1000 + (long long)tv.tv_usec/1000;
    else
        timestamp = tv.tv_sec;

    if (pszTS != NULL)
    {
        int2string(timestamp, szT);
        strcpy(pszTS, szT);
    }

    return timestamp;
}

int get_hash_code_24(char* psz_combined_string)
{
	int hash_code = 0;
	if (psz_combined_string == NULL || strlen(psz_combined_string) <= 0)
		return 0;

    uint i = 0;
	for (i = 0; i < strlen(psz_combined_string); i++)
	{
		hash_code = ((hash_code << 5) - hash_code) + (int)psz_combined_string[i];
		hash_code = hash_code & 0x00FFFFFF;
	}
	return hash_code;
}
