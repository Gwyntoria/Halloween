/*
 * Detection.c:
 *
 * By Jessica Mao 2020/04/17
 *
 * Copyright (c) 2012-2020 Lotogram Inc. <lotogram.com, zhuagewawa.com>

 * Version 1.0.0.72	Details in update.log
 ***********************************************************************
 */

#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <opencv2/core/core.hpp>
#include "Detection.h"
#include "WaInit.h"

using std::vector;
static char* template_image_file[5] = {(char*)"mojie_template.png", (char*)"wanshengye_template.png", (char*)"haiwang2_template.png", (char*)"chongchongbuluo2_template.png", (char*)"haiwang3_template.png"};
static char* wanshengye_empty_template_file = (char*)"wanshengye_empty_cannon.png";
static char* haiwang_empty_template_file = (char*)"haiwang_empty_cannon.png";
static char* littlemonster_template_file = (char*)"wanshengye_little_monster.png";
static char* baoji_template_file = (char*)"wanshengye_baoji.png";
static char* weapons_template_file[6] = {(char*)"wanshengye_moxuandan.png", (char*)"wanshengye_dianguangpo.png", (char*)"wanshengye_lianhuanbaolie.png", (char*)"wanshengye_suolianfengyin.png", (char*)"wanshengye_qumolieyan.png", (char*)"wanshengye_mofazhaohuan.png"};
static char* monsters_template_file[6] = {(char*)"wanshengye_huanyingsishen.png", (char*)"wanshengye_nanguaguai.png", (char*)"wanshengye_kuangbaofeilong.png", (char*)"wanshengye_xiuluojumo.png", (char*)"wanshengye_kuloubaojun.png", (char*)"wanshengye_jushiyanmo.png"};


static Rect monster_rects[6][4] = {{Rect(100, 540, 60, 50), Rect(420, 540, 60, 50), Rect(740, 540, 60, 50), Rect(1060, 540, 60, 50)},   //huanyingsishen
                                   {Rect(100, 470, 60, 50), Rect(420, 470, 60, 50), Rect(740, 470, 60, 50), Rect(1060, 470, 60, 50)},   //nanguaguai
                                   {Rect(100, 540, 60, 50), Rect(420, 540, 60, 50), Rect(740, 540, 60, 50), Rect(1060, 540, 60, 50)},   //kuangbaofeilong
                                   {Rect(45, 540, 60, 50), Rect(365, 540, 60, 50), Rect(685, 540, 60, 50), Rect(1005, 540, 60, 50)},   //xiuluojumo
                                   {Rect(100, 540, 60, 50), Rect(420, 540, 60, 50), Rect(740, 540, 60, 50), Rect(1060, 540, 60, 50)},   //kuloubaojun
                                   {Rect(45, 540, 60, 50), Rect(365, 540, 60, 50), Rect(685, 540, 60, 50), Rect(1005, 540, 60, 50)}};  //jushiyanmo

static Rect weapon_rects[6][4] = {{Rect(200, 375, 60, 55), Rect(520, 375, 60, 55), Rect(840, 375, 60, 55), Rect(1160, 375, 60, 55)},
                                  {Rect(200, 375, 60, 55), Rect(520, 375, 60, 55), Rect(840, 375, 60, 55), Rect(1160, 375, 60, 55)},
                                  {Rect(105, 480, 60, 60), Rect(425, 480, 60, 60), Rect(745, 480, 60, 60), Rect(1065, 480, 60, 60)},
                                  {Rect(105, 480, 60, 60), Rect(425, 480, 60, 60), Rect(745, 480, 60, 60), Rect(1065, 480, 60, 60)},
                                  {Rect(105, 430, 60, 60), Rect(425, 430, 60, 60), Rect(745, 430, 60, 60), Rect(1065, 430, 60, 60)},
                                  {Rect(105, 490, 60, 60), Rect(425, 490, 60, 60), Rect(745, 490, 60, 60), Rect(1065, 490, 60, 60)}};


static Rect detect_rect[5][8] = {{Rect(168, 684, 102, 26), Rect(443, 684, 102, 26), Rect(741, 684, 102, 26), Rect(1016, 684, 102, 26)},
                                  {Rect(188, 680, 122, 40), Rect(508, 680, 122, 40), Rect(828, 680, 122, 40), Rect(1148, 680, 122, 40)},
                                  {Rect(220, 680, 100, 40), Rect(540, 680, 100, 40), Rect(860, 680, 100, 40), Rect(1180, 680, 100, 40)},
                                  {Rect(12, 685, 123, 29), Rect(322, 685, 123, 29), Rect(632, 685, 123, 29), Rect(942, 685, 123, 29),
                                   Rect(1142, 5, 123, 29), Rect(832, 5, 123, 29), Rect(522, 5, 123, 29), Rect(212, 5, 123, 29)},
                                  {Rect(220, 680, 100, 40), Rect(540, 680, 100, 40), Rect(860, 680, 100, 40), Rect(1180, 680, 100, 40),
                                   Rect(960, 0, 105, 45), Rect(640, 0, 105, 45), Rect(320, 0, 105, 45), Rect(0, 0, 105, 45)}};

static Rect board_error_rect = Rect(565, 340, 55, 35);
static Rect border_rect[4] = {Rect(285, 675, 25, 45), Rect(605, 675, 25, 45), Rect(925, 675, 25, 45), Rect(1245, 675, 25, 45)};

static Rect wanshengye_strength_rect[4] = {Rect(138, 690, 45, 30), Rect(458, 690, 45, 30), Rect(778, 690, 45, 30), Rect(1098, 690, 45, 30)};
static Rect haiwang_strength_rect[8] = {Rect(140, 680, 40, 20), Rect(460, 680, 40, 20), Rect(780, 680, 40, 20), Rect(1100, 680, 40, 20),
                                        Rect(1100, 20, 40, 20), Rect(780, 20, 40, 20), Rect(460, 20, 40, 20), Rect(140, 20, 40, 20)};

static Rect kuangbaofeilong_rect[4] = {Rect(100, 440, 60, 100), Rect(420, 440, 60, 100), Rect(740, 440, 60, 100), Rect(1060, 440, 60, 100)};//

static Rect little_monster_rect[4] = {Rect(0, 480, 130, 90), Rect(320, 480, 130, 90), Rect(640, 480, 130, 90), Rect(960, 480, 130, 90)};

static Rect baoji_rect[4] = {Rect(150, 510, 90, 70), Rect(470, 510, 90, 70), Rect(790, 510, 90, 70), Rect(1110, 510, 90, 70)};

static Rect wanshengye_check_empty_rect[4] = {Rect(205, 670, 55, 50), Rect(525, 670, 55, 50), Rect(850, 670, 55, 50), Rect(1165, 670, 55, 50)};
static Rect haiwang_check_empty_rect[8] = {Rect(265, 670, 25, 50), Rect(585, 670, 25, 50), Rect(905, 670, 25, 50), Rect(1225, 670, 25, 50),
                                        Rect(990, 0, 25, 50), Rect(670, 0, 25, 50), Rect(350, 0, 25, 50), Rect(30, 0, 25, 50)};

static char* monster_map_value[] = {(char*)"幻影死神", (char*)"千年南瓜怪", (char*)"狂暴飞龙", (char*)"修罗巨魔", (char*)"骷髅暴君", (char*)"巨石炎魔"};
static char* weapon_map_value[] = {(char*)"魔旋弹", (char*)"电光破", (char*)"连环爆裂", (char*)"锁链封印", (char*)"驱魔烈焰", (char*)"魔法召唤"};
static char* little_monster_map_value[] = {(char*)"床单幽灵", (char*)"伞怪", (char*)"南瓜怪", (char*)"木乃伊", (char*)"死神", (char*)"西洋僵尸", (char*)"中国僵尸", (char*)"人造人", (char*)"吸血鬼"};
static int strength_value[] = {50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000};

#define DETECT_THRESHOLD    0.60

struct DETECT_DATA
{
    int x;
    float   fThreshold;
    int iNum;
};

static int  s_subtype = 0;
static int  s_player_count = 0;
static Mat  s_template_image;
static Mat  s_strength_template_image;
static Mat  s_border_template_image;
static Mat  s_empty_template_image;
static Mat  s_board_error_template_image;
static Mat  s_weapon_template_image;
static Mat  s_nanguaguai_template_image;
static Mat  s_littlemonster_template_image;
static Mat  s_baoji_template_image;
static Mat  s_monster_template_image;
static Mat  s_monster_template_images[6];
static Mat  s_weapon_template_images[6];
static char s_current_folder[256];

static bool data_compare(const DETECT_DATA &infoA,const DETECT_DATA &infoB)
{
    if (infoA.x < infoB.x)
        return true;
    return  false;
}

static int ocr_number_template(Mat source_image, int iPosition)
{
    int source_height = source_image.rows;
    int source_width = source_image.cols;
    int i_tmp_width = s_template_image.cols;
    int i_tmp_height = s_template_image.rows;
    int i_cell_height = i_tmp_height/s_player_count;
    int i_cell_width = i_tmp_width/10;
    if (s_subtype == 4)
        i_cell_height = i_tmp_height/8;

    if (source_height < i_cell_height || source_width < i_cell_width)
    {
        LOGD ("[%s] [OCR Template] source_image is less than template_image, Position %d", log_Time(), iPosition);
        return -3;
    }

    Rect crop_rect(0, (iPosition - 1) * i_cell_height, i_tmp_width, i_cell_height);
	Mat crop_reference(s_template_image, crop_rect);

    vector<DETECT_DATA> location_data;

    try
    {
        for (int i = 0; i < 10; i ++)
        {
            Rect crop_rect(i * i_cell_width, 0, i_cell_width, i_cell_height);

            Mat cell_reference(crop_reference, crop_rect);
            Mat result;

            matchTemplate(source_image, cell_reference, result, TM_CCOEFF_NORMED);

            DETECT_DATA detect_data;
            for (int nRow = 0; nRow < result.rows; nRow ++)
            {
                for (int nCol = 0; nCol < result.cols; nCol ++)
                {
                    float fTemp = result.at<float>(nRow, nCol);
                    if (fTemp > DETECT_THRESHOLD) //阈值比较
                    {
                        detect_data.x= nCol;
                        detect_data.fThreshold = fTemp;
                        detect_data.iNum = i;
                        location_data.push_back(detect_data);
                    }
                }
            }
            cell_reference.release();
            result.release();
        }
    }
    catch (cv::Exception const & e) {
        LOGD ("[%s] [OCR Number template] cv::exception: %s", log_Time(), e.what());
    }
    catch (...) {
        LOGD ("[%s] [OCR Number template] other exception", log_Time());
    }

    crop_reference.release();

    if (location_data.size() > 0)
    {
        sort(location_data.begin(), location_data.end(), data_compare);

        vector<DETECT_DATA> group_data;
        DETECT_DATA pre_data = location_data[0];
        for (int i = 1; i < location_data.size(); i ++)
        {
            if ((location_data[i].x - pre_data.x) < 10)
            {
                if(location_data[i].fThreshold > pre_data.fThreshold)
                    pre_data = location_data[i];
            }
            else
            {
                group_data.push_back(pre_data);
                pre_data = location_data[i];
            }
        }

        group_data.push_back(pre_data);
        sort(group_data.begin(), group_data.end(), data_compare);

        if (iPosition > 4)
        {
            for (int i = 0; i < group_data.size()/2; i ++)
            {
                swap(group_data[i], group_data[group_data.size()-i-1]);
            }
        }

        char szNumber[32] = "";
        for (int i = 0; i < group_data.size(); i ++)
        {
            sprintf(szNumber, "%s%d", szNumber, group_data[i].iNum);
        }

        //LOGD("[%s] number: %s", log_Time(), szNumber);
        return  string2int(szNumber);
    }
    else
        return  -1;
}

static int ocr_image_template(Mat source_image, Mat template_image, int divide_num, int iPosition, float threshold = DETECT_THRESHOLD, int is_strength = 0)
{
    int i_tmp_width = template_image.cols;
    int i_tmp_height = template_image.rows;
    int i_cell_height = i_tmp_height;
    int i_cell_width = i_tmp_width/divide_num;
    int i_cell_x = 0;

    Mat crop_reference;
    bool croped = false;
    if (is_strength == 1)
    {
        if (s_subtype == 2 || s_subtype == 4)
        {
            i_cell_height = i_tmp_height/2;
            Rect crop_rect(0, 0, i_tmp_width, i_cell_height);
            if (iPosition >= 5)
                crop_rect.y = i_cell_height;
            crop_reference = Mat(template_image, crop_rect);
            croped = true;
        }
        else {
            crop_reference = template_image;
        }
    }
    else if (iPosition >= 1 && iPosition <= s_player_count)
    {
        i_cell_height = i_tmp_height/s_player_count;
        Rect crop_rect(0, (iPosition - 1) * i_cell_height, i_tmp_width, i_cell_height);
        crop_reference = Mat(template_image, crop_rect);
        croped = true;
    }
    else
    {
        crop_reference = template_image;
    }

    vector<DETECT_DATA> location_data;

    try
    {
        for (int i = 0; i < divide_num; i ++)
        {
            Rect crop_rect(i_cell_x, 0, i_cell_width, i_cell_height);
            i_cell_x = i_cell_x + i_cell_width;

            Mat cell_reference(crop_reference, crop_rect);
            Mat result;

            matchTemplate(source_image, cell_reference, result, TM_CCOEFF_NORMED);

            DETECT_DATA detect_data;
            for (int nRow = 0; nRow < result.rows; nRow ++)
            {
                for (int nCol = 0; nCol < result.cols; nCol ++)
                {
                    float fTemp = result.at<float>(nRow, nCol);
                    if (fTemp > threshold) //阈值比较
                    {
                        detect_data.x= nCol;
                        detect_data.fThreshold = fTemp;
                        detect_data.iNum = i;
                        location_data.push_back(detect_data);
                    }
                }
            }
            cell_reference.release();
            result.release();
        }
    }
    catch (cv::Exception const & e) {
        LOGD ("[%s] [OCR Image template] cv::exception: %s", log_Time(), e.what());
    }
    catch (...) {
        LOGD ("[%s] [OCR Image template] other exception", log_Time());
    }

    if (croped == true)
        crop_reference.release();

    if (location_data.size() > 0)
    {
        sort(location_data.begin(), location_data.end(), data_compare);

        vector<DETECT_DATA> group_data;
        DETECT_DATA pre_data = location_data[0];
        for (int i = 1; i < location_data.size(); i ++)
        {
            if ((location_data[i].x - pre_data.x) < 5)
            {
                if(location_data[i].fThreshold > pre_data.fThreshold)
                    pre_data = location_data[i];
            }
            else
            {
                group_data.push_back(pre_data);
                pre_data = location_data[i];
            }
        }

        group_data.push_back(pre_data);
        sort(group_data.begin(), group_data.end(), data_compare);

        char szNumber[32] = "";
        for (int i = 0; i < group_data.size(); i ++)
        {
            sprintf(szNumber, "%s%d", szNumber, group_data[i].iNum);
        }

       //LOGD("[%s] number: %s", log_Time(), szNumber);
        return  string2int(szNumber);
    }
    else
        return  -1;
}

int get_strength_value(Mat source_image, int iPosition)
{
    if (source_image.empty())
    {
        LOGD ("[%s] [Get Strength Value] Param error: source image is None", log_Time());
        return  -1;
    }

    if (s_subtype != 1 && s_subtype != 2 && s_subtype != 4)
    {
        LOGD ("[%s] [Get Strength Value] Not Support", log_Time());
        return  -1;
    }

    if (iPosition < 1 || iPosition > s_player_count)
    {
        LOGD ("[%s] [Get Strength Value] Param error: Position is %d", log_Time(), iPosition);
        return  -1;
    }

    Rect crop_rect;
    if (s_subtype == 1)
        crop_rect = wanshengye_strength_rect[iPosition-1];
    else if (s_subtype == 2 || s_subtype == 4)
        crop_rect = haiwang_strength_rect[iPosition-1];

    int source_height = source_image.rows;
    int source_width = source_image.cols;
    if (crop_rect.x >= source_width || crop_rect.y >= source_height || (crop_rect.x+crop_rect.width) > source_width || (crop_rect.y+crop_rect.height) > source_height)
    {
        LOGD ("[%s] [Get Strength Value] Param error: Source Image is small, Position is %d", log_Time(), iPosition);
        return -1;
    }

    Mat crop_image(source_image, crop_rect);
    int strength_value_size = sizeof(strength_value)/sizeof(int);
    int i_index = ocr_image_template(crop_image, s_strength_template_image, strength_value_size, iPosition, 0.8, 1);
    crop_image.release();
    if (i_index >= 0 and i_index < strength_value_size)
        return strength_value[i_index];
    else
        return 0;

}

static int ocr_monster_type_template(Mat source_image, Mat template_image, int divide_num, float threshold = DETECT_THRESHOLD)
{
    int i_tmp_width = template_image.cols;
    int i_tmp_height = template_image.rows;
    int i_cell_height = i_tmp_height;
    int i_cell_width = i_tmp_width/divide_num;
    int i_cell_x = 0;

    Mat crop_reference = template_image;
    vector<DETECT_DATA> location_data;

    try
    {
        for (int i = 0; i < divide_num; i ++)
        {
            Rect crop_rect(i_cell_x, 0, i_cell_width, i_cell_height);
            i_cell_x = i_cell_x + i_cell_width;

            Mat cell_reference(crop_reference, crop_rect);
            Mat result;

            matchTemplate(source_image, cell_reference, result, TM_CCOEFF_NORMED);

            DETECT_DATA detect_data;
            for (int nRow = 0; nRow < result.rows; nRow ++)
            {
                for (int nCol = 0; nCol < result.cols; nCol ++)
                {
                    float fTemp = result.at<float>(nRow, nCol);
                    if (fTemp > threshold) //阈值比较
                    {
                        detect_data.x= nCol;
                        detect_data.fThreshold = fTemp;
                        detect_data.iNum = i;
                        location_data.push_back(detect_data);
                        break;
                    }
                }
                if (location_data.size() > 0)
                    break;
            }
            cell_reference.release();
            result.release();

            if (location_data.size() > 0)
                break;
        }
    }
    catch (cv::Exception const & e) {
        LOGD ("[%s] [OCR Image template] cv::exception: %s", log_Time(), e.what());
    }
    catch (...) {
        LOGD ("[%s] [OCR Image template] other exception", log_Time());
    }

    if (location_data.size() > 0)
    {
        sort(location_data.begin(), location_data.end(), data_compare);

        vector<DETECT_DATA> group_data;
        DETECT_DATA pre_data = location_data[0];
        for (int i = 1; i < location_data.size(); i ++)
        {
            if ((location_data[i].x - pre_data.x) < 5)
            {
                if(location_data[i].fThreshold > pre_data.fThreshold)
                    pre_data = location_data[i];
            }
            else
            {
                group_data.push_back(pre_data);
                pre_data = location_data[i];
            }
        }

        group_data.push_back(pre_data);
        sort(group_data.begin(), group_data.end(), data_compare);

        char szNumber[32] = "";
        for (int i = 0; i < group_data.size(); i ++)
        {
            sprintf(szNumber, "%s%d", szNumber, group_data[i].iNum);
        }

       //LOGD("[%s] number: %s", log_Time(), szNumber);
        return  string2int(szNumber);
    }
    else
        return  -1;
}

int get_weapon_type(Mat source_image, int iPosition)
{
    if (source_image.empty())
    {
        LOGD ("[%s] [Get Weapon Type] Param error: source image is None", log_Time());
        return  -1;
    }

    if (s_subtype != 1)
    {
        LOGD ("[%s] [Get Weapon Type] Not Support", log_Time());
        return  -1;
    }

    if (iPosition < 1 || iPosition > s_player_count)
    {
        LOGD ("[%s] [Get Weapon Type] Param error: Position is %d", log_Time(), iPosition);
        return  -1;
    }

    int weapon_size = sizeof(weapon_map_value)/sizeof(char*);
    for (int i = 0; i < weapon_size; i ++)
    {
        Rect crop_rect = weapon_rects[i][iPosition-1];

        int source_height = source_image.rows;
        int source_width = source_image.cols;
        if (crop_rect.x >= source_width || crop_rect.y >= source_height || (crop_rect.x+crop_rect.width) > source_width || (crop_rect.y+crop_rect.height) > source_height)
        {
            LOGD ("[%s] [Get Weapon Type] Param error: Source Image is small, Position is %d", log_Time(), iPosition);
            return -1;
        }

        Mat crop_image(source_image, crop_rect);
        int i_index = ocr_monster_type_template(crop_image, s_weapon_template_images[i], 1, 0.8);
        crop_image.release();
        if (i_index == 0)
            return i;
    }
    return  -1;
}

int get_littlemonster_type(Mat source_image, int iPosition)
{
    if (source_image.empty())
    {
        LOGD ("[%s] [Get Little Monster Type] Param error: source image is None", log_Time());
        return  -1;
    }

    if (s_subtype != 1)
    {
        LOGD ("[%s] [Get Little Monster Type] Not Support", log_Time());
        return  -1;
    }

    if (iPosition < 1 || iPosition > s_player_count)
    {
        LOGD ("[%s] [Get Little Monster Type] Param error: Position is %d", log_Time(), iPosition);
        return  -1;
    }

    Rect crop_rect = little_monster_rect[iPosition-1];
    int source_height = source_image.rows;
    int source_width = source_image.cols;
    if (crop_rect.x >= source_width || crop_rect.y >= source_height || (crop_rect.x+crop_rect.width) > source_width || (crop_rect.y+crop_rect.height) > source_height)
    {
        LOGD ("[%s] [Get Little Monster Type] Param error: Source Image is small, Position is %d", log_Time(), iPosition);
        return -1;
    }

    Mat crop_image(source_image, crop_rect);
    int little_monster_size = sizeof(little_monster_map_value)/sizeof(char*);
    int i_index = ocr_monster_type_template(crop_image, s_littlemonster_template_image, little_monster_size, 0.85);
    crop_image.release();
    return i_index;
}

int get_monster_type(Mat source_image, int iPosition)
{
    if (source_image.empty())
    {
        LOGD ("[%s] [Get Monster Type] Param error: source image is None", log_Time());
        return  -1;
    }
    if (iPosition < 1 || iPosition > s_player_count)
    {
        LOGD ("[%s] [Get Monster Type] Param error: Position is %d", log_Time(), iPosition);
        return  -1;
    }

    int monster_size = sizeof(monster_map_value)/sizeof(char*);
    for (int i = 0; i < monster_size; i ++)
    {
        Rect crop_rect = monster_rects[i][iPosition-1];
        int source_height = source_image.rows;
        int source_width = source_image.cols;
        if (crop_rect.x >= source_width || crop_rect.y >= source_height || (crop_rect.x+crop_rect.width) > source_width || (crop_rect.y+crop_rect.height) > source_height)
        {
            LOGD ("[%s] [Get Monster Type] Param error: Source Image is small, Position is %d", log_Time(), iPosition);
            return -1;
        }
        Mat crop_image(source_image, crop_rect);

        int i_index = ocr_monster_type_template(crop_image, s_monster_template_images[i], 1, 0.8);
        crop_image.release();
        if (i_index == 0)
            return i;
        else if (i == 2)
        {
            Rect crop_rect = kuangbaofeilong_rect[iPosition-1];
            int source_height = source_image.rows;
            int source_width = source_image.cols;
            if (crop_rect.x >= source_width || crop_rect.y >= source_height || (crop_rect.x+crop_rect.width) > source_width || (crop_rect.y+crop_rect.height) > source_height)
            {
                LOGD ("[%s] [Get Monster Type] Param error: Source Image is small, Position is %d", log_Time(), iPosition);
                return -1;
            }
            Mat crop_image(source_image, crop_rect);

            int i_index = ocr_monster_type_template(crop_image, s_monster_template_images[i], 1, 0.8);
            crop_image.release();
            if (i_index == 0)
                return  i;
        }
    }
    return  -1;
}

static int ocr_empty_cannon_template(Mat source_image, Mat template_image, int divide_num, int iPosition, float threshold = DETECT_THRESHOLD)
{
    int i_tmp_width = template_image.cols;
    int i_tmp_height = template_image.rows;
    int i_cell_width = i_tmp_width/divide_num;

    Rect crop_rect((iPosition - 1) * i_cell_width, 0, i_cell_width, i_tmp_height);
    Mat crop_reference = Mat(template_image, crop_rect);
    vector<DETECT_DATA> location_data;
    Mat result;

    try
    {
        matchTemplate(source_image, crop_reference, result, TM_CCOEFF_NORMED);
    }
    catch (cv::Exception const & e) {
        LOGD ("[%s] [OCR Emtpy template] cv::exception: %s", log_Time(), e.what());
    }
    catch (...) {
        LOGD ("[%s] [OCR Emtpy template] other exception", log_Time());
    }

    DETECT_DATA detect_data;
    for (int nRow = 0; nRow < result.rows; nRow ++)
    {
        for (int nCol = 0; nCol < result.cols; nCol ++)
        {
            float fTemp = result.at<float>(nRow, nCol);
            if (fTemp > threshold) //阈值比较
            {
                detect_data.x= nCol;
                detect_data.fThreshold = fTemp;
                detect_data.iNum = 0;
                location_data.push_back(detect_data);
            }
        }
    }
    crop_reference.release();
    result.release();

    if (location_data.size() > 0)
    {
        sort(location_data.begin(), location_data.end(), data_compare);

        vector<DETECT_DATA> group_data;
        DETECT_DATA pre_data = location_data[0];
        for (int i = 1; i < location_data.size(); i ++)
        {
            if ((location_data[i].x - pre_data.x) < 5)
            {
                if(location_data[i].fThreshold > pre_data.fThreshold)
                    pre_data = location_data[i];
            }
            else
            {
                group_data.push_back(pre_data);
                pre_data = location_data[i];
            }
        }

        group_data.push_back(pre_data);
        sort(group_data.begin(), group_data.end(), data_compare);

        char szNumber[32] = "";
        for (int i = 0; i < group_data.size(); i ++)
        {
            sprintf(szNumber, "%s%d", szNumber, group_data[i].iNum);
        }

        //LOGD("[%s] number: %s", log_Time(), szNumber);
        return  string2int(szNumber);
    }
    else
        return  -4;
}

bool check_empty_cannon(Mat source_image, int iPosition)
{
    if (source_image.empty())
    {
        LOGD ("[%s] [Check Empty Cannon] Param error: source image is None", log_Time());
        return  -1;
    }
    if (iPosition < 1 || iPosition > s_player_count)
    {
        LOGD ("[%s] [Check Empty Cannon] Param error: Position is %d", log_Time(), iPosition);
        return  false;
    }

    if (s_subtype == 1 || s_subtype == 2 || s_subtype == 4)
    {
        Rect crop_rect;
        if (s_subtype == 1)
        {
            crop_rect = wanshengye_check_empty_rect[iPosition-1];
        }
        else if (s_subtype == 2 || s_subtype == 4)
        {
            crop_rect = haiwang_check_empty_rect[iPosition-1];
        }
        int source_height = source_image.rows;
        int source_width = source_image.cols;
        if (crop_rect.x >= source_width || crop_rect.y >= source_height || (crop_rect.x+crop_rect.width) > source_width || (crop_rect.y+crop_rect.height) > source_height)
        {
            LOGD ("[%s] [Check Empty Cannon] Param error: Source Image is small, Position is %d", log_Time(), iPosition);
            return false;
        }
        Mat crop_image(source_image, crop_rect);
        int value = 0;
        if (s_subtype == 1)
        {
            value = ocr_empty_cannon_template(crop_image, s_empty_template_image, 4, iPosition, 0.8);
        }
        else
        {
            value = ocr_empty_cannon_template(crop_image, s_empty_template_image, 8, iPosition, 0.8);
        }
        crop_image.release();
        if (value == 0)
            return true;

        return false;
    }
    else {
        LOGD ("[%s] [Check Empty Cannon] not wanshengye or haiwang", log_Time());
        return false;
    }
}

bool check_board_error(Mat source_image)
{
    if (source_image.empty())
    {
        LOGD ("[%s] [Check Board Error] Param error: source image is None", log_Time());
        return  false;
    }

    Rect crop_rect = board_error_rect;
    int source_height = source_image.rows;
    int source_width = source_image.cols;
    if (crop_rect.x >= source_width || crop_rect.y >= source_height || (crop_rect.x+crop_rect.width) > source_width || (crop_rect.y+crop_rect.height) > source_height)
    {
        LOGD ("[%s] [Check Board Error] Param error: Source Image is small", log_Time());
        return false;
    }
    Mat crop_image(source_image, crop_rect);
    int value = ocr_monster_type_template(crop_image, s_board_error_template_image, 1, 0.7);
    crop_image.release();
    if (value == 0)
        return true;

    return false;
}

bool check_baoji_status(Mat source_image, int iPosition)
{
    if (source_image.empty())
    {
        LOGD ("[%s] [Check Baoji Status] Param error: source image is None", log_Time());
        return  -1;
    }
    if (iPosition < 1 || iPosition > s_player_count)
    {
        LOGD ("[%s] [Check Baoji Status] Param error: Position is %d", log_Time(), iPosition);
        return  -1;
    }

    Rect crop_rect = baoji_rect[iPosition-1];
    int source_height = source_image.rows;
    int source_width = source_image.cols;
    if (crop_rect.x >= source_width || crop_rect.y >= source_height || (crop_rect.x+crop_rect.width) > source_width || (crop_rect.y+crop_rect.height) > source_height)
    {
        LOGD ("[%s] [Check Baoji Status] Param error: Source Image is small, Position is %d", log_Time(), iPosition);
        return -1;
    }
    Mat crop_image(source_image, crop_rect);
    int i_index = ocr_monster_type_template(crop_image, s_baoji_template_image, 1, 0.9);
    crop_image.release();
    if (i_index == 0)
        return true;

    return false;
}

int read_score_by_image(Mat source_image, int iPosition)
{
    if (source_image.empty())
    {
        LOGD ("[%s] [Read Score By Image] Param error: source image is None", log_Time());
        return  -1;
    }
    if (iPosition < 1 || iPosition > s_player_count)
    {
        LOGD ("[%s] [Read Score By Image] Param error: Position is %d", log_Time(), iPosition);
        return  -1;
    }

    if (s_subtype == 1)
    {
        Rect crop_rect = border_rect[iPosition-1];
        Mat crop_image(source_image, crop_rect);
        if (ocr_image_template(crop_image, s_border_template_image, 1, iPosition) != 0)
        {
            LOGD ("[%s] [Read Score By Image] Position is %d, border is not full!", log_Time(), iPosition);
            crop_image.release();
            return -1;
        }
        crop_image.release();
    }

    Rect crop_rect = detect_rect[s_subtype][iPosition-1];
    int source_height = source_image.rows;
    int source_width = source_image.cols;
    if (crop_rect.x >= source_width || crop_rect.y >= source_height || (crop_rect.x+crop_rect.width) > source_width || (crop_rect.y+crop_rect.height) > source_height)
    {
        LOGD ("[%s] [Read Score By Image] Param error: Source Image is small, Position is %d", log_Time(), iPosition);
        return -1;
    }

    Mat crop_image(source_image, crop_rect);
    int iScore = ocr_number_template(crop_image, iPosition);
    crop_image.release();
    return iScore;
}

char* get_monster_name(int i_monster_type)
{
    return  monster_map_value[i_monster_type];
}

char* get_weapon_name(int i_weapon_type)
{
    return  weapon_map_value[i_weapon_type];
}

char* get_littlemonster_name(int i_littlemonster_type)
{
    return  little_monster_map_value[i_littlemonster_type];
}

void init_detection(int iSubType, int iPlayerCount)
{
    s_subtype = iSubType;
    s_player_count = iPlayerCount;

    sprintf(s_current_folder, "%sWaController/LibOCR/", WORK_FOLDER);
    char sz_template_file[512];
    sprintf(sz_template_file, "%s%s", s_current_folder, template_image_file[iSubType]);
    s_template_image = imread(sz_template_file, 1);

    if (iSubType == 1)
    {
        sprintf(sz_template_file, "%swanshengye_strength_template.png", s_current_folder);
        s_strength_template_image = imread(sz_template_file, 1);
        sprintf(sz_template_file, "%swanshengye_border_template.png", s_current_folder);
        s_border_template_image = imread(sz_template_file, 1);

        sprintf(sz_template_file, "%s%s", s_current_folder, wanshengye_empty_template_file);
        s_empty_template_image = imread(sz_template_file, 1);

        sprintf(sz_template_file, "%s%s", s_current_folder, littlemonster_template_file);
        s_littlemonster_template_image = imread(sz_template_file, 1);

        sprintf(sz_template_file, "%s%s", s_current_folder, baoji_template_file);
        s_baoji_template_image = imread(sz_template_file, 1);

        for (int i = 0; i < 6; i ++)
        {
            sprintf(sz_template_file, "%s%s", s_current_folder, monsters_template_file[i]);
            s_monster_template_images[i] = imread(sz_template_file, 1);
        }

        for (int i = 0; i < 6; i ++)
        {
            sprintf(sz_template_file, "%s%s", s_current_folder, weapons_template_file[i]);
            s_weapon_template_images[i] = imread(sz_template_file, 1);
        }
    }
    else if (iSubType == 2 || iSubType == 4)
    {
        sprintf(sz_template_file, "%shaiwang_strength_template.png", s_current_folder);
        s_strength_template_image = imread(sz_template_file, 1);
        sprintf(sz_template_file, "%s%s", s_current_folder, haiwang_empty_template_file);
        s_empty_template_image = imread(sz_template_file, 1);
    }

    sprintf(sz_template_file, "%sboard_error.png", s_current_folder);
    s_board_error_template_image = imread(sz_template_file, 1);
}

void uninit_detection()
{
    s_template_image.release();
    if (s_subtype == 1)
    {
        s_strength_template_image.release();
        s_border_template_image.release();
        s_empty_template_image.release();
        s_littlemonster_template_image.release();
        s_baoji_template_image.release();
        for (int i = 0; i < 6; i ++)
        {
            s_monster_template_images[i].release();
        }

        for (int i = 0; i < 6; i ++)
        {
            s_weapon_template_images[i].release();
        }
    }
    else if (s_subtype == 2 || s_subtype == 4)
    {
        s_empty_template_image.release();
    }
    s_board_error_template_image.release();
}
