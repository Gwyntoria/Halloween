// #ifdef __cplusplus
// #if __cplusplus
// extern "C"{
// #endif
// #endif /* End of #ifdef __cplusplus */

#include <vector>
#include <stdio.h>
#include <unistd.h>
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/types_c.h"
#include <sys/time.h>
#include <sys/timeb.h>

using std::vector;

#define DETECT_THRESHOLD    0.60

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

struct DETECT_DATA
{
    int x;
    float   fThreshold;
    int iNum;
};

static cv::Rect detect_rect[4] = {cv::Rect(188, 680, 122, 40), cv::Rect(508, 680, 122, 40), cv::Rect(828, 680, 122, 40), cv::Rect(1148, 680, 122, 40)};

static char s_current_folder[256];
static cv::Mat  s_template_image;


static bool data_compare(const DETECT_DATA &infoA,const DETECT_DATA &infoB)
{
    if (infoA.x < infoB.x)
        return true;
    return  false;
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

static int ocr_number_template(cv::Mat source_image, int iPosition)
{
    int source_height = source_image.rows;
    int source_width = source_image.cols;
    int i_tmp_width = s_template_image.cols;
    int i_tmp_height = s_template_image.rows;
    int i_cell_height = i_tmp_height/4;
    int i_cell_width = i_tmp_width/10;

    if (source_height < i_cell_height || source_width < i_cell_width)
    {
        return -3;
    }

    cv::Rect crop_rect(0, (iPosition - 1) * i_cell_height, i_tmp_width, i_cell_height);
	cv::Mat crop_reference(s_template_image, crop_rect);

    // cv::imwrite("/root/WaController/template_image.jpg", crop_reference);

    vector<DETECT_DATA> location_data;

    try
    {
        for (int i = 0; i < 10; i ++)
        {
            cv::Rect crop_rect(i * i_cell_width, 0, i_cell_width, i_cell_height);

            cv::Mat cell_reference(crop_reference, crop_rect);
            cv::Mat result;

            // cv::imwrite("/root/WaController/0.jpg", cell_reference);

            printf("before matchTemplate i = %d: %s\n", i, log_Time());
            cv::matchTemplate(source_image, cell_reference, result, cv::TM_CCOEFF_NORMED);
            printf("end matchTemplate i = %d: %s\n", i, log_Time());

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
    }
    catch (...) {
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
                std::swap(group_data[i], group_data[group_data.size()-i-1]);
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


int read_score_by_image(cv::Mat source_image, int iPosition)
{
    if (source_image.empty())
    {
        return  -1;
    }
    if (iPosition < 1 || iPosition > 4)
    {
        return  -1;
    }

    cv::Rect crop_rect = detect_rect[iPosition-1];
    int source_height = source_image.rows;
    int source_width = source_image.cols;
    if (crop_rect.x >= source_width || crop_rect.y >= source_height || (crop_rect.x+crop_rect.width) > source_width || (crop_rect.y+crop_rect.height) > source_height)
    {
        return -1;
    }

    cv::Mat crop_image(source_image, crop_rect);
    // cv::imwrite("/root/WaController/crop_image.jpg", crop_image);

    printf("before ocr_number_template: %s\n", log_Time());
    int iScore = ocr_number_template(crop_image, iPosition);
    printf("end ocr_number_template: %s\n", log_Time());
    crop_image.release();
    return iScore;
}

void init_detection()
{
    strcpy(s_current_folder, "/root/WaController/");
    char sz_template_file[512];
    sprintf(sz_template_file, "%swanshengye_template.png", s_current_folder);
    s_template_image = cv::imread(sz_template_file, 1);
}

int main(int argc, char *argv[])
{
    // char image_path[256] = "/root/WaController/1.jpg";
    // char save_image_path[256] = "/root/WaController/1_save.jpg";
	// IplImage* pimg = cvLoadImage(image_path, CV_LOAD_IMAGE_COLOR);
    // cvSaveImage(save_image_path, pimg, 0);
    // cvReleaseImage(&pimg);

    init_detection();

    cv::Mat sourceImage = cv::imread("/root/WaController/980.jpg", 1);
    printf("before read_score_by_image: %s\n", log_Time());
    int score = read_score_by_image(sourceImage, 1);
    printf("end read_score_by_image: %s\n", log_Time());

    printf("score: %d\n", score);

	return 0;
}

// #ifdef __cplusplus
// #if __cplusplus
// }
// #endif
// #endif /* End of #ifdef __cplusplus */