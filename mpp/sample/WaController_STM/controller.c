/*
 * controller.c:
 *
 * By Jessica Mao 2020/04/24
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include "WaInit.h"
#include "controller.h"
#include "lotoSerial.h"


static int s_sockfd_client;
static int s_iRoomSubType;

#define CMD_HEADER          0x88
#define CMD_END             0x55
#define CMD_TYPE_REQUEST    0x01
#define CMD_TYPE_RESPONSE   0x02

static char cmd_connect[7] = { 0x88, 0x04, 0x01, 0x01, 0x01, 0x07, 0x55 };
static char cmd_disconnect[6] = { 0x88, 0x03, 0x02, 0x01, 0x06, 0x55 };
static char cmd_controller[4] = { 0x88, 0x13, 0x03, 0x01 };

//static char C1Char_temp[16] = {0};
static char C1Char [15] = { 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };     // hai wang 2

static char d_C1Char [5][15] = { { 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, \
                            { 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, \
                            { 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, \
                            { 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, \
                            { 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03 }};

static char system_C1Char [5][15] = { { 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, \
								{ 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6 }, \
								{ 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6 }, \
								{ 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF6 }, \
                                { 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02 }};

#define KEY_UP 		0xFE
#define KEY_DOWN 	0xFD
#define KEY_LEFT 	0xF7
#define KEY_RIGHT 	0xFB
#define KEY_FIRE 	0xDF
#define KEY_ENHANCE 0xEF
#define KEY_ADD 	0x7F

// 1~8bit is key mapping
static char key_option_map[7] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_FIRE, KEY_ENHANCE, KEY_ADD};

// 9~11bit is subtraction of coins
static char key_sub_array[8][3] = { { 0xFE, 0xFF, 0xFF }, 
									{ 0xF7, 0xFF, 0xFF }, 
									{ 0xBF, 0xFF, 0xFF }, 
									{ 0xFF, 0xFD, 0xFF }, 
									{ 0xFF, 0xEF, 0xFF }, 
									{ 0xFF, 0x7F, 0xFF }, 
									{ 0xFF, 0xFF, 0xFB }, 
									{ 0xFF, 0xFF, 0xDF }};

static int s_c1_length = 15;
static int s_sub_index = 9;
static int s_fd = 0;
static int s_is_connected = 0;
static pthread_t s_receive_thread_id = 0;
static pthread_t s_send_thread_id = 0;

// 13bit is clear error
#define KEY_CLEAR_ERROR_1P 	0xFE
#define KEY_CLEAR_ERROR_2P 	0xFD
#define KEY_CLEAR_ERROR_3P 	0xFB
#define KEY_CLEAR_ERROR_4P 	0xF7
#define KEY_CLEAR_ERROR_5P 	0xEF
#define KEY_CLEAR_ERROR_6P  0xDF
#define KEY_CLEAR_ERROR_7P 	0xBF
#define KEY_CLEAR_ERROR_8P 	0x7F

#define OPTION_GAME_AB 	0x0100     		//AB组合键
#define OPTION_GAME_X 	0x0101      	//P2 A&B, P1 A  台湾主机的隐藏功能

static void get_hex_string(char* hex_string, char char_array[], int array_length)
{
    hex_string[array_length * 3] = '\0';
    for (int i = 0; i < array_length; i ++)
        sprintf(hex_string + i * 3, " %02X", char_array[i]);
}

static void send_global_key()
{
    static int sending = 0;
    char C1Char_temp[16] = {0};

    if (sending == 1)
    {
        //LOGD("[%s] Send Global Key is sending", log_Time());
        return;
    }

    memcpy(C1Char_temp, C1Char, s_c1_length);
    if (C1Char_temp[0] == 0xC1)
    {
        sending = 1;
        int check_sum = 0;
        int i = 0;
        for (i = 0; i < s_c1_length; i ++)
        {
            check_sum += C1Char_temp[i];
        }
        C1Char_temp[s_c1_length] = check_sum & 0xFF;

        int cmd_length = 6+s_c1_length+1;
        char cmd_temp[22] = {0};
        bzero(cmd_temp, sizeof(char)*22);
        memcpy(cmd_temp, cmd_controller, 4);
        memcpy(cmd_temp+4, C1Char_temp, s_c1_length+1);
        cmd_temp[1] = s_c1_length+1+3;

        check_sum = 0;
        for (i = 1; i < cmd_length-2; i ++)
        {
            check_sum += cmd_temp[i];
        }
        cmd_temp[cmd_length-2] = check_sum & 0xFF;
        cmd_temp[cmd_length-1] = 0x55;

        serialPuts(cmd_temp, cmd_length);

//        char sz_hex_string[256] = {0};
//        get_hex_string(sz_hex_string, cmd_temp, cmd_length);
//        LOGE ("[%s] [CONTROLLER] Send Data to SMT: %s", log_Time(), sz_hex_string);

        sending = 0;
    }
    else
    {
        LOGD("[%s] Check Send Global key, C1 Error", log_Time());
    }
}

void controller_user_drop(int iPosition)
{
    if (iPosition >= 1 && iPosition <= 8)
	{
		C1Char[iPosition] = (C1Char[iPosition] & KEY_ADD) & 0xFF;
        send_global_key();
        usleep(30000);
		C1Char[iPosition] = (C1Char[iPosition] | (~KEY_ADD)) & 0xFF;
        send_global_key();
	}
}

void controller_user_option_ab(int iPosition)
{
    C1Char[iPosition] = (C1Char[iPosition] & KEY_FIRE) & 0xFF;
    C1Char[iPosition] = (C1Char[iPosition] & KEY_ENHANCE) & 0xFF;
    send_global_key();
    usleep(35000);
    C1Char[iPosition] = (C1Char[iPosition] | (~KEY_FIRE)) & 0xFF;
    C1Char[iPosition] = (C1Char[iPosition] | (~KEY_ENHANCE)) & 0xFF;
    send_global_key();
}

void controller_user_option_click(int iPosition, int iOption)
{
    C1Char[iPosition] = (C1Char[iPosition] & key_option_map[iOption]) & 0xFF;
    send_global_key();
    usleep(35000);
    C1Char[iPosition] = (C1Char[iPosition] | (~key_option_map[iOption])) & 0xFF;
    send_global_key();
}

void controller_user_option_down(int iPosition, int iOption)
{
    C1Char[iPosition] = (C1Char[iPosition] & key_option_map[iOption]) & 0xFF;
    send_global_key();
}

void controller_user_option_up(int iPosition, int iOption)
{
    C1Char[iPosition] = (C1Char[iPosition] | (~key_option_map[iOption])) & 0xFF;
    send_global_key();
}

void controller_user_shoot(int iPosition)
{
    C1Char[iPosition] = (C1Char[iPosition] & KEY_FIRE) & 0xFF;
    send_global_key();
    usleep(35000);
    C1Char[iPosition] = (C1Char[iPosition] | (~KEY_FIRE)) & 0xFF;
    send_global_key();
}

void controller_shoots(int iPositions[], int iCount)
{
    if (iCount <= 0)
		return;

	int i = 0;
	for (i = 0; i < iCount; i++)
	{
	    C1Char[iPositions[i]] = (C1Char[iPositions[i]] & KEY_FIRE) & 0xFF;

//	    LOGD ("[%s] [controller_shoots] iPosition = %d", log_Time(), iPositions[i]);
	}
    send_global_key();
    usleep(35000);
	for (i = 0; i < iCount; i++)
	{
		C1Char[iPositions[i]] = (C1Char[iPositions[i]] | (~KEY_FIRE)) & 0xFF;
	}
    send_global_key();
}

void controller_user_shoot_strength(int iPosition)
{
    C1Char[iPosition] = (C1Char[iPosition] & KEY_ENHANCE) & 0xFF;
    send_global_key();
    usleep(35000);
    C1Char[iPosition] = (C1Char[iPosition] | (~KEY_ENHANCE)) & 0xFF;
    send_global_key();
}

void controller_user_clear_score(int iPosition)
{
	C1Char[s_sub_index] = (C1Char[s_sub_index] & key_sub_array[iPosition - 1][0]) & 0xFF;
	C1Char[s_sub_index+1] = (C1Char[s_sub_index+1] & key_sub_array[iPosition - 1][1]) & 0xFF;
	if (s_iRoomSubType != 0)
	    C1Char[s_sub_index+2] = (C1Char[s_sub_index+2] & key_sub_array[iPosition - 1][2]) & 0xFF;
    send_global_key();
    usleep(3500000);
	C1Char[s_sub_index] = (C1Char[s_sub_index] | (~key_sub_array[iPosition - 1][0])) & 0xFF;
	C1Char[s_sub_index+1] = (C1Char[s_sub_index+1] | (~key_sub_array[iPosition - 1][1])) & 0xFF;
	if (s_iRoomSubType != 0)
	    C1Char[s_sub_index+2] = (C1Char[s_sub_index+2] | (~key_sub_array[iPosition - 1][2])) & 0xFF;
    send_global_key();
}

// clear all scores or clean wrong posion scores
void controller_admin_clear_scores(int iPositions[], int iCount)
{
	if (iCount <= 0)
		return;

	int i = 0;
	for (i = 0; i < iCount; i++)
	{
		C1Char[s_sub_index] = (C1Char[s_sub_index] & key_sub_array[iPositions[i] - 1][0]) & 0xFF;
		C1Char[s_sub_index+1] = (C1Char[s_sub_index+1] & key_sub_array[iPositions[i] - 1][1]) & 0xFF;
		if (s_iRoomSubType != 0)
		    C1Char[s_sub_index+2] = (C1Char[s_sub_index+2] & key_sub_array[iPositions[i] - 1][2]) & 0xFF;
	}
    send_global_key();
    usleep(3500000);
	for (i = 0; i < iCount; i++)
	{
		C1Char[s_sub_index] = (C1Char[s_sub_index] | (~key_sub_array[iPositions[i] - 1][0])) & 0xFF;
		C1Char[s_sub_index+1] = (C1Char[s_sub_index+1] | (~key_sub_array[iPositions[i] - 1][1])) & 0xFF;
		if (s_iRoomSubType != 0)
		    C1Char[s_sub_index+2] = (C1Char[s_sub_index+2] | (~key_sub_array[iPositions[i] - 1][2])) & 0xFF;
	}
    send_global_key();
}

void controller_add_scores(int iPositions[], int iCount)
{
    if (iCount <= 0)
		return;

	int i = 0;
	for (i = 0; i < iCount; i++)
	{
	    C1Char[iPositions[i]] = (C1Char[iPositions[i]] & KEY_ADD) & 0xFF;
	}
    send_global_key();
    usleep(30000);
	for (i = 0; i < iCount; i++)
	{
		C1Char[iPositions[i]] = (C1Char[iPositions[i]] | (~KEY_ADD)) & 0xFF;
	}
    send_global_key();
}

void controller_admin_options(int iOption)
{
	if (iOption == OPTION_GAME_AB)
	{
		C1Char[1] = (C1Char[1] & KEY_FIRE) & 0xFF;
		C1Char[1] = (C1Char[1] & KEY_ENHANCE) & 0xFF;
        send_global_key();
        usleep(35000);
		C1Char[1] = (C1Char[1] | (~KEY_FIRE)) & 0xFF;
		C1Char[1] = (C1Char[1] | (~KEY_ENHANCE)) & 0xFF;
        send_global_key();
	}
	else if (iOption == OPTION_GAME_X)
	{
		C1Char[2] = (C1Char[2] & KEY_FIRE) & 0xFF;
		C1Char[2] = (C1Char[2] & KEY_ENHANCE) & 0xFF;
        send_global_key();
        usleep(50000);
		C1Char[1] = (C1Char[1] & KEY_FIRE) & 0xFF;
        send_global_key();
        usleep(200000);
		C1Char[1] = (C1Char[1] | (~KEY_FIRE)) & 0xFF;
		C1Char[2] = (C1Char[2] | (~KEY_ENHANCE)) & 0xFF;
		C1Char[2] = (C1Char[2] | (~KEY_FIRE)) & 0xFF;
        send_global_key();
	}
	else
	{
		C1Char[1] = (C1Char[1] & key_option_map[iOption]) & 0xFF;
        send_global_key();
        usleep(35000);
		C1Char[1] = (C1Char[1] | (~key_option_map[iOption])) & 0xFF;
        send_global_key();
	}
}

void controller_admin_open_settings()
{
	memcpy(C1Char, system_C1Char[s_iRoomSubType], s_c1_length);
    send_global_key();
    usleep(35000);
	memcpy(C1Char, d_C1Char[s_iRoomSubType], s_c1_length);
    send_global_key();
}

static void* receive_data_thread(void *inPtr)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    while(1)
    {
        char readByte[256];
        bzero(readByte, 256);

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
        int nread = serialReadline(readByte);
        if (readByte[0] == CMD_HEADER)
        {
            int data_length = readByte[1];
            if (nread >= (data_length+3))
            {
                int check_sum = 0;
                for (int i = 1; i <= data_length; i++)
                {
                    check_sum += readByte[i];
                }
                check_sum &= 0xFF;
                if (check_sum == readByte[data_length+1] && readByte[data_length+2] == CMD_END)
                {
                    if (readByte[3] == CMD_TYPE_RESPONSE)
                    {
                        switch (readByte[2])
                        {
                            case    0x01:
                                LOGE ("[%s] [CONTROLLER] connected", log_Time()) ;
                                s_is_connected = 1;
                                break;
                            case    0x02:
                                LOGE ("[%s] [CONTROLLER] disconnected", log_Time()) ;
                                break;
                            case    0x03:
                                LOGE ("[%s] [CONTROLLER] SMT has received controller key", log_Time()) ;
                                break;
                            default:
                                break;
                        }

                    }
                }
            }
        }
        else {

        }

        char sz_hex_string[256] = {0};
        get_hex_string(sz_hex_string, readByte, nread);
        LOGE ("[%s] [CONTROLLER] Receive Data From SMT: %s", log_Time(), sz_hex_string);

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        pthread_testcancel();
    }
    return NULL;
}

static void* send_connect_thread(void *inPtr)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

    while (s_is_connected == 0)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
        cmd_connect[4] = s_iRoomSubType+1;
        int check_sum = 0;
        for (int i = 1; i < 5; i ++)
        {
            check_sum += cmd_connect[i];
        }
        cmd_connect[5] = check_sum & 0xFF;
        serialPuts(cmd_connect, 7);

        char sz_hex_string[256] = {0};
        get_hex_string(sz_hex_string, cmd_connect, 7);
        LOGE ("[%s] [CONTROLLER] Send Connect to SMT: %s", log_Time(), sz_hex_string);

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        pthread_testcancel();
        sleep(2);
        pthread_testcancel();
    }
    return NULL;
}

void controller_init(int iRoomSubType)
{
	s_iRoomSubType = iRoomSubType;
	s_c1_length = s_iRoomSubType == 0 ? 11 : 15;
	s_sub_index = s_iRoomSubType == 0 ? 7 : 9;
    memcpy(C1Char, d_C1Char[s_iRoomSubType], s_c1_length);

    if ((s_fd = serial485Open ("/dev/ttyAMA0", 115200, 8, 'O', 2)) < 0)
    {
        LOGE ("[%s] Unable to open serial device: %s", log_Time(), strerror (errno)) ;
    }

    if (s_receive_thread_id != 0)
        pthread_cancel(s_receive_thread_id);
    if (s_send_thread_id != 0)
        pthread_cancel(s_send_thread_id);

    if(pthread_create(&s_receive_thread_id,NULL,receive_data_thread,NULL) != 0)
    {
        LOGE ("[%s] [CONTROLLER] No receive data thread", log_Time());
    }

    if(pthread_create(&s_send_thread_id,NULL,send_connect_thread,NULL) != 0)
    {
        LOGE ("[%s] [CONTROLLER] No Send Connect thread", log_Time());
    }
}

