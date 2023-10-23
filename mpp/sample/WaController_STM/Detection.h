/*
 * WaInit.h:
 *
 ***********************************************************************
 * by Jessica Mao
 * Lotogram Inc,. 2020/04/17
 *
 ***********************************************************************
 */

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifndef DETECTION_H
#define DETECTION_H

#ifdef __cplusplus
extern "C" {
#endif

using namespace cv;

void init_detection(int iSubType, int iPlayerCount);
void uninit_detection();
int read_score_by_image(Mat source_image, int iPosition);
int get_strength_value(Mat source_image, int iPosition);
int get_monster_type(Mat source_image, int iPosition);
int get_weapon_type(Mat source_image, int iPosition);
int get_littlemonster_type(Mat source_image, int iPosition);
char* get_monster_name(int i_monster_type);
char* get_weapon_name(int i_weapon_type);
char* get_littlemonster_name(int i_littlemonster_type);
bool check_empty_cannon(Mat source_image, int iPosition);
bool check_board_error(Mat source_image);
bool check_baoji_status(Mat source_image, int iPosition);

#ifdef __cplusplus
}
#endif

#endif
