/*
 * WaInit.c:
 *
 * By Jessica Mao 2020/04/17
 *
 * Copyright (c) 2012-2020 Lotogram Inc. <lotogram.com, zhuagewawa.com>

 * Version 1.0.0.72	Details in update.log
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
#include <sys/ioctl.h>
#include <time.h>
#include "http.h"
#include "md5.h"
#include "cJSON.h"
#include "WaInit.h"
#include "vector.h"
#include <vector>
#include <algorithm>
#include <string>

using namespace std;
using std::vector;

int parse_jsonstring(loto_room_info* pRoomInfo, const char* pszResponse)
{
	if (pRoomInfo == NULL || pszResponse == NULL)
		return -1;

	cJSON *json = cJSON_Parse(pszResponse);

	if ( NULL != json )
	{
		cJSON * pItem = cJSON_GetObjectItem(json, "status");
		if ( NULL != pItem )
		{
			strcpy(pRoomInfo->szStatus, pItem->valuestring);
		}

		pItem = cJSON_GetObjectItem(json, "code");
		if ( NULL != pItem )
		{
			pRoomInfo->iCode = pItem->valueint;
		}

		cJSON * pRoomItem = cJSON_GetObjectItem(json, "room");
		if ( NULL != pRoomItem )
		{
			pItem = cJSON_GetObjectItem(pRoomItem, "type");
			if ( NULL != pItem )
			{
				pRoomInfo->iRoomType = pItem->valueint;
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "subtype");
			if ( NULL != pItem )
			{
				pRoomInfo->iSubType = pItem->valueint;
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "encodetype");
			if ( NULL != pItem )
			{
				pRoomInfo->iEncodeType = pItem->valueint;
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "monster");
			if ( NULL != pItem )
			{
				pRoomInfo->iSupportMonster = pItem->valueint;
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "machtype");
			if ( NULL != pItem )
			{
				pRoomInfo->iMatchType = pItem->valueint;
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "vip");
			if ( NULL != pItem )
			{
				pRoomInfo->iVip = pItem->valueint;
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "jp");
			if ( NULL != pItem )
			{
				pRoomInfo->iJP = pItem->valueint;
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "_id");
			if ( NULL != pItem )
			{
				strcpy(pRoomInfo->szRoomId, pItem->valuestring);
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "domain");
			if ( NULL != pItem )
			{
				strcpy(pRoomInfo->szDomain, pItem->valuestring);
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "name");
			if ( NULL != pItem )
			{
				strcpy(pRoomInfo->szName, pItem->valuestring);
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "hostname");
			if ( NULL != pItem )
			{
				strcpy(pRoomInfo->szHostName, pItem->valuestring);
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "routerip");
			if ( NULL != pItem )
			{
				strcpy(pRoomInfo->szRouterIp, pItem->valuestring);
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "websocket");
			if ( NULL != pItem )
			{
				strcpy(pRoomInfo->szWebsocket, pItem->valuestring);
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "ip");
			if ( NULL != pItem )
			{
				strcpy(pRoomInfo->szIp, pItem->valuestring);
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "routerdns");
			if ( NULL != pItem )
			{
				strcpy(pRoomInfo->szRouterDNS, pItem->valuestring);
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "frpport");
			if ( NULL != pItem )
			{
				strcpy(pRoomInfo->szFrpPort, pItem->valuestring);
			}
			pItem = cJSON_GetObjectItem(pRoomItem, "frps");
			if ( NULL != pItem )
			{
				strcpy(pRoomInfo->szFrps, pItem->valuestring);
			}
			cJSON* pRtmpItem = cJSON_GetObjectItem(pRoomItem, "rtmp");
			if ( NULL != pRtmpItem )
			{
				pItem = cJSON_GetObjectItem(pRtmpItem, "pushurl");
				if ( NULL != pItem )
				{
					strcpy(pRoomInfo->szPushURL, pItem->valuestring);
				}
				pItem = cJSON_GetObjectItem(pRtmpItem, "screenshot");
				if ( NULL != pItem )
				{
					strcpy(pRoomInfo->szScreenShot, pItem->valuestring);
				}
                pItem = cJSON_GetObjectItem(pRtmpItem, "ip");
				if ( NULL != pItem )
				{
					strcpy(pRoomInfo->szRTMPIp, pItem->valuestring);
				}

			}
			cJSON* pGameItem = cJSON_GetObjectItem(pRoomItem, "game");
			if ( NULL != pGameItem )
			{
				pItem = cJSON_GetObjectItem(pGameItem, "count");
				if ( NULL != pItem )
				{
					pRoomInfo->iPlayerCount = pItem->valueint;
				}
			}
		}

		cJSON_Delete(json);
		json = NULL;

		return 0;
	}
	return -1;
}

cJSON* get_frp_info()
{
    loto_room_info*    pRoomInfo = NULL;
    char c[100];
    std::string response;
    int index = 0;
    char sz_cmd_line[512];
    sprintf(sz_cmd_line, "python3 %sWaController/room_frp.pyc", WORK_FOLDER);
    FILE *fp = popen(sz_cmd_line,"r");
    while (fgets(c, sizeof(c), fp) != NULL)
    {
        response = response + c;
    }
    pclose(fp);

	if (response.length() > 0)
	{
		replace(response.begin(),response.end(),'\'','\"');
		printf("response: %s\n", response.c_str());

		return cJSON_Parse((const char*)response.c_str());
	}

    return NULL;
}

loto_room_info* loto_room_init()
{
	loto_room_info*    pRoomInfo = NULL;
    char c[100];
    std::string response;
    int index = 0;
    char sz_cmd_line[512];
    sprintf(sz_cmd_line, "python3 %sWaController/room_register.pyc", WORK_FOLDER);
    FILE *fp = popen(sz_cmd_line, "r");
    while (fgets(c, sizeof(c), fp) != NULL)
    {
        response = response + c;
    }
    pclose(fp);

	if (response.length() > 0)
	{
		replace(response.begin(),response.end(),'\'','\"');
		printf("response: %s\n", response.c_str());

        pRoomInfo = (loto_room_info*)malloc(sizeof(loto_room_info));
        memset(pRoomInfo, sizeof(loto_room_info), 0);
        int iRet = parse_jsonstring(pRoomInfo, response.c_str());
        if (iRet != 0)
        {
            free(pRoomInfo);
			pRoomInfo = NULL;
			return NULL;
        }

		FILE *pFile = fopen("/etc/dhcpcd.conf", "r");
		if(pFile != NULL)
		{
			char	szLine[512] = "";
			int 	iDHCPFind = 0;
			Vector* vertor = vector_new(512);
			while(!feof(pFile))
			{
				if(ferror(pFile))
			    {
			        perror("error");
			        break;
			    }
			    strcpy(szLine, "");
				fgets(szLine, 512, pFile);
				if (strlen(szLine) <= 0)
					continue;
				if (strstr(szLine, "127.0.0.1") != NULL)
					continue;
				if (strstr(szLine, "Inform the DHCP") != NULL)
				{
					vector_append(vertor, szLine);
					iDHCPFind = 1;
					continue;
				}
				if (strstr(szLine, "hostname") != NULL && strlen(szLine) < 10)
				{
					char szTmp[256];
					sprintf(szTmp, "%s\n", pRoomInfo->szHostName);
					vector_append(vertor, szTmp);
					continue;
				}
				if (strstr(szLine, "ip_address=") != NULL)
				{
					char szTmp[256];
					sprintf(szTmp, "static ip_address=%s/22\n", pRoomInfo->szIp);
					vector_append(vertor, szTmp);
					continue;
				}
				if (strstr(szLine, "routers=") != NULL)
				{
					char szTmp[256];
					sprintf(szTmp, "static routers=%s\n", pRoomInfo->szRouterIp);
					vector_append(vertor, szTmp);
					continue;
				}
				 if (strstr(szLine, "domain_name_servers=") != NULL)
				 {
				 	char szTmp[256];
				 	sprintf(szTmp, "static domain_name_servers=%s\n", pRoomInfo->szRouterDNS);
				 	vector_append(vertor, szTmp);
				 	continue;
				 }
				if (iDHCPFind == 1)
				{
					char szTmp[256];
					sprintf(szTmp, "%s\n", pRoomInfo->szHostName);
					vector_append(vertor, szTmp);
					iDHCPFind = 0;
					continue;
				}
				vector_append(vertor, szLine);
			}
			fclose(pFile);

			pFile = fopen("/etc/dhcpcd.conf", "w");
			if (pFile != NULL)
			{
				//printf("write dhcpdc.conf\n");
                uint i = 0;

				for (i = 0; i < vector_length(vertor); i ++ )
				{
					vector_get(vertor, i, szLine);
					//printf("Line_Write: %s", szLine);
					fputs(szLine, pFile);
				}
				fclose(pFile);
			}
			vector_free(vertor);
		}

		pFile = fopen("/etc/hosts", "r");
		if (pFile != NULL)
		{
			char	szLine[512] = "";
			Vector* vertor = vector_new(512);
			while(!feof(pFile))
			{
				strcpy(szLine, "");
				fgets(szLine, 512, pFile);
				//printf("Line hosts: %s", szLine);
				if (strlen(szLine) <= 0)
				    continue;
				if (strstr(szLine, "127.0.0.1") != NULL && strstr(szLine, "localhost") == NULL)
				    continue;
				vector_append(vertor, szLine);
			}
			char szTmp[256];
			sprintf(szTmp, "127.0.0.1       %s\n", pRoomInfo->szHostName);
			vector_append(vertor, szTmp);
			fclose(pFile);

			pFile = fopen("/etc/hosts", "w");
			if (pFile != NULL)
			{
                uint i = 0;
				for (i = 0; i < vector_length(vertor); i ++ )
				{
					vector_get(vertor, i, szLine);
					fputs(szLine, pFile);
				}
				fclose(pFile);
			}
			vector_free(vertor);
		}
		pFile = fopen("/etc/hostname", "w");
		if (pFile != NULL)
		{
			char szTmp[256];
			sprintf(szTmp, "%s\n", pRoomInfo->szHostName);
            //printf("Line_Write hostname: %s", szTmp);
			fputs(szTmp, pFile);
			fclose(pFile);
		}

		char    szFrp_addr[64] = "";
		char    szFrp_port[32] = "";
		if (strlen(pRoomInfo->szFrps) > 0)
		{
		    char* pszTemp = strchr(pRoomInfo->szFrps, ':');
		    strncpy(szFrp_addr, pRoomInfo->szFrps, strlen(pRoomInfo->szFrps)-strlen(pszTemp));
		    strcpy(szFrp_port, pszTemp+1);
		}

        char sz_file_name[512];
        sprintf(sz_file_name, "%sfrp_0.13.0_linux_arm/frpc.ini", WORK_FOLDER);
        pFile = fopen(sz_file_name, "r");
		if (pFile != NULL)
		{
			char	szLine[512] = "";
			Vector* vertor = vector_new(512);
			while(!feof(pFile))
			{
				strcpy(szLine, "");
				fgets(szLine, 512, pFile);
		        //printf("Line frpc: %s", szLine);
				if (strlen(szLine) <= 0)
					continue;

				if (strlen(szFrp_addr) > 0 && strstr(szLine, "server_addr") != NULL)
				{
				    char szTmp[256];
			        sprintf(szTmp, "server_addr=%s\n", szFrp_addr);
			        vector_append(vertor, szTmp);
				}
				else if (strlen(szFrp_port) > 0 && strstr(szLine, "server_port") != NULL)
				{
				    char szTmp[256];
			        sprintf(szTmp, "server_port=%s\n", szFrp_port);
			        vector_append(vertor, szTmp);
				}
                else
				    vector_append(vertor, szLine);
				if (strstr(szLine, "log_max_days") != NULL)
				{
					vector_append(vertor, (char*)"\n");
					break;
				}

			}
			char szTmp[256];
			sprintf(szTmp, "[%s]\n", pRoomInfo->szFrpPort);
			vector_append(vertor, szTmp);
			sprintf(szTmp, "type = tcp\n");
			vector_append(vertor, szTmp);
			sprintf(szTmp, "local_ip = 127.0.0.1\n");
			vector_append(vertor, szTmp);
			sprintf(szTmp, "local_port = 22\n");
			vector_append(vertor, szTmp);
			sprintf(szTmp, "remote_port = %s\n", pRoomInfo->szFrpPort);
			vector_append(vertor, szTmp);

			fclose(pFile);

			pFile = fopen(sz_file_name, "w");
			if (pFile != NULL)
			{
                uint i = 0;

				for (i = 0; i < vector_length(vertor); i ++ )
				{
					vector_get(vertor, i, szLine);
					//printf("Line_Write frpc: %s", szLine);
					fputs(szLine, pFile);
				}
				fclose(pFile);
			}
			vector_free(vertor);
		}

        sprintf(sz_file_name, "%sWaController/config.ini", WORK_FOLDER);
        pFile = fopen(sz_file_name, "w");
		if (pFile != NULL)
		{
			char szTmp[256];

			fputs("[room]\n", pFile);
            sprintf(szTmp, "roomtype=%d\n", pRoomInfo->iRoomType);
			fputs(szTmp, pFile);
            sprintf(szTmp, "subtype=%d\n", pRoomInfo->iSubType);
			fputs(szTmp, pFile);
			fclose(pFile);
		}
	}
    return pRoomInfo;
}