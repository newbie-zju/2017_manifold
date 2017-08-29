#include "shapeRecg.h"

namespace irobot
{
ShapeRecg::ShapeRecg()
{
    iLowHr1 = 0; 
    iHighHr1 = 20;
    iLowHr2 = 160;
    iHighHr2 = 180;
    iLowSr = 80;
    iHighSr = 255;
    iLowVr = 30;
    iHighVr = 255;
    NumofPoints = 54; //固定
    
    ros::param::get("~LowHg", iLowHg);
    ros::param::get("~HighHg", iHighHg);
    ros::param::get("~LowSg", iLowSg);
    ros::param::get("~HighSg", iHighSg);
    ros::param::get("~LowVg", iLowVg);
    ros::param::get("~HighVg", iHighVg);
    
    /*iLowHg = 10;   //hsv模型的上下限
    iHighHg = 100;
    iLowSg = 15;
    iHighSg = 255;
    iLowVg = 10;
    iHighVg = 100;*/
    
    ros::param::get("~shapeSim_threshold", shapeSim_thresha);
    ros::param::get("~referenceShapeNum", refShapeNum);
    //ros::param::get("~Sampsuth", path);
    //shapeSim_thresha = 0.1;
    //refShapeNum = 3;
}
ShapeRecg::~ShapeRecg()
{
  ROS_INFO("Destroying ShapeRecg......");
}
std::string ShapeRecg::int2str(int &i)
{
    std::string s;
    std::stringstream ss(s);
    ss << i;
    return ss.str();
}

void ShapeRecg::showImg(const cv::Mat &Img, const std::string &WindowName, bool BGRornot, int ColorcvtCode)
{
    cv::Mat BGRImg;
    if (BGRornot)
    {
        BGRImg = Img;
    }
    else
    {
        cv::cvtColor(Img, BGRImg, ColorcvtCode);
    }
    cv::namedWindow(WindowName, cv::WINDOW_AUTOSIZE);
    cv::imshow(WindowName, BGRImg);
    cv::waitKey(1);
    return;
}

void ShapeRecg::preprocess(const cv::Mat &Img, cv::Mat &HSVImg)
{
    //高斯滤波可选，中值滤波第三个参数可调，HSV直方图规定可选
    cv::Mat BlrImg;
    cv::medianBlur(Img, BlrImg, 3);
    cv::cvtColor(BlrImg, HSVImg, cv::COLOR_BGR2HSV);
    return;
}

void ShapeRecg::recgColor(const cv::Mat &SrcImg, const cv::Mat &HSVImg, cv::Mat &TargImg)
{
    cv::Mat TempColorTargImg, TempGrayTargImg;
    cv::inRange(HSVImg, cv::Scalar(iLowHr1, iLowSr, iLowVr), cv::Scalar(iHighHr1, iHighSr, iHighVr), TargImg);
    cv::inRange(HSVImg, cv::Scalar(iLowHr2, iLowSr, iLowVr), cv::Scalar(iHighHr2, iHighSr, iHighVr), TempColorTargImg);
    TargImg = (cv::max)(TargImg, TempColorTargImg);

    cv::inRange(HSVImg, cv::Scalar(iLowHg, iLowSg, iLowVg), cv::Scalar(iHighHg, iHighSg, iHighVg), TempColorTargImg);
    // //不用明暗度只用Shape对比，以下是采用灰度的代码
    //cv::Mat GrayImg;
    //cvtColor(SrcImg, GrayImg, CV_BGR2GRAY);
    //threshold(GrayImg, TempGrayTargImg, grayThresh, 255, THRESH_BINARY_INV);
    //TargImg = max(TargImg, min(TempColorTargImg, TempGrayTargImg));
    TargImg = (cv::max)(TargImg, TempColorTargImg);//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
   // cv::namedWindow("TargImg");
   // cv::imshow("TarImg", TargImg);
    //cv::waitKey(10);
    cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(TargImg, TargImg, cv::MORPH_OPEN, element);
    cv::morphologyEx(TargImg, TargImg, cv::MORPH_CLOSE, element);
    
    return;
}

void ShapeRecg::getContours(const cv::Mat &TargImg, std::vector<std::vector<cv::Point> > &contours)
{
    cv::blur(TargImg, TargImg, cv::Size(3, 3));
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(TargImg, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
    return;
}

void ShapeRecg::getRefShape(std::vector<std::vector<cv::Point> > &RefContours, int NumofPoints, int NumofRefs)
{
    std::string path = "/home/ubuntu/2017/src/irobot_detect/Refshapes/";
    char szFileName[100];
    for (int p = 0; p < NumofRefs; p++)
    {
        int q = p + 1;
	cv::Mat TemplateImg = cv::imread(path + "Refshapes (" + int2str(q) + ").png", 0);
	//sprintf(szFileName,"/home/alex/iarc2017_ws/src/irobot_detect/Refshapes/Refshapes (%d).png",q);
	//cv::Mat TemplateImg = cv::imread(szFileName,0);
	std::vector<std::vector<cv::Point> > Contours;
	cv::Point Center;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(TemplateImg, Contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
	RefContours.push_back(shapecontext_obj.getSampledPoints(Contours[0], NumofPoints));
    }
    return;
}

void ShapeRecg::filterShape(std::vector<std::vector<cv::Point> > &Contours, const std::vector<std::vector<cv::Point> > &RefContours, std::vector<int> &d, int minNumofPoints, double DisSimilarityThresh)
{
    for (int i = (int)Contours.size() - 1; i >= 0; i--)
    {
        if (Contours[i].size() < minNumofPoints)
        {
            Contours.erase(Contours.begin() + i);
	}
    }
    
    std::vector<ShapeContext::Hist> RefHistograms;
    for (int p = 0; p < RefContours.size(); p++)
    {
        RefHistograms.push_back(shapecontext_obj.getHistogramFromContourPts(RefContours[p]));
    }
    
    for (int i = (int)Contours.size() - 1; i >= 0; i--)
    {
        int size = (int)RefContours[0].size();
        Contours[i] = shapecontext_obj.getSampledPoints(Contours[i], (int)RefContours[0].size());
	ShapeContext::Hist Histogram = shapecontext_obj.getHistogramFromContourPts(Contours[i]);
	double minDissimilarity = HUGE_VAL;
	int dtp;
	for (int p = 0; p < RefContours.size(); p++)
	{
	    double **chiStatistics = (double **)malloc(sizeof(double *) * size);
	    for (int i = 0; i < size; i++)
	    {
	        chiStatistics[i] = (double *)malloc(sizeof(double) * size);
                for (int j = 0; j < size; j++)
		{
		  chiStatistics[i][j] = -1.0;
		}    
	    }
	    shapecontext_obj.getChiStatistic(chiStatistics, RefHistograms[p], Histogram, size);
            int *Ref2Shape = (int *)malloc(sizeof(int) * size);
            std::pair<double, int> DistanceIdx;
            DistanceIdx = shapecontext_obj.minMatchingCost(size, chiStatistics, Ref2Shape);
            if (minDissimilarity > DistanceIdx.first)
            {
                minDissimilarity = DistanceIdx.first;
            }
            if (p == 0)
	    {
                dtp = DistanceIdx.second;
            }
            for (int i = 0; i < size; i++) 
	    {
	        free(chiStatistics[i]);
	    }
            free(chiStatistics);
            free(Ref2Shape);
            if (minDissimilarity > (1.7*DisSimilarityThresh))
            {
                break;
            }
        }
        
        delete[] Histogram;
        if (minDissimilarity > DisSimilarityThresh)
        {
            Contours.erase(Contours.begin() + i);
	}
	else
	{
            d.push_back(dtp);
	}
    }
    for(size_t p=0;p<RefHistograms.size();p++)
      delete[] RefHistograms[p];
    return;
}

std::vector<std::vector<cv::Point> > ShapeRecg::shaperecg(cv::Mat SrcImg, bool debug)
{
    cv::Mat HSVImg;
    preprocess(SrcImg, HSVImg);
    cv::Mat TargImg;
    recgColor(SrcImg, HSVImg, TargImg);

    
    cv::morphologyEx(TargImg, TargImg, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));//开运算防止轮廓重叠，最后两个参数可调
    std::vector<std::vector<cv::Point> > RefContours;
    getRefShape(RefContours, NumofPoints, refShapeNum);//第三个参数为Refshapes文件夹里现有Refshape的个数,第一个永远为最标准的shape，因八个角点只由第一个Ref得到
    
    std::vector <std::vector<cv::Point> > Contours;
    getContours(TargImg, Contours);
    int minNumofPoints = 85;//边界大小阈值
    std::vector<int> d;
    ShapeRecg::filterShape(Contours, RefContours, d, minNumofPoints, shapeSim_thresha);//第三到第四个参数待调整
    std::vector<int> RefcornerPointsIdx = {0,2,7,14,31,39,43,46};//NumofPoints为54时候的角点

/*
RefcornerPointsIdx中每个点代表的位置，Results每一子向量里地面机器人中的八个点也依次如下给出
               7----------0
               |          |
               |          |
   5-----------6          1-----------2
   |                                  |
   |                                  |
   |                                  |
   |                                  |
   4----------------------------------3

*/

    std::vector<std::vector<cv::Point> > Results;
    for (int p = 0; p < Contours.size(); p++)
    {
        std::vector<cv::Point> cornerPoints;
	for (int q = 0; q < RefcornerPointsIdx.size(); q++)
	{
	    cornerPoints.push_back(Contours[Contours.size() - p - 1][(d[p] + RefcornerPointsIdx[q]) % NumofPoints]);
	  
	}
	
	Results.push_back(cornerPoints);
    }
    if (1)//debug状态下立即显示结果
    {
        //ShapeRecg::showImg(SrcImg, "Image");
	//ShapeRecg::showImg(TargImg, "Result Image", 0, CV_GRAY2BGR);
	// Draw contours,彩色轮廓
	cv::Mat dst = cv::Mat::zeros(TargImg.size(), CV_8UC3);
	//RNG rng;
	for (int i = 0; i < Contours.size(); i++)
	{
	    cv::drawContours(dst, Contours, i, cv::Scalar(255, 0, 0), 2, 8);
	    for (int p = 0; p < 8; p++)
	    {
	        cv::circle(dst, Results[i][p], 5, cv::Scalar(255, 255, 255));
	    }
	}
	// Create Window  
	char* source_window = "countors";
	//cv::namedWindow(source_window, CV_WINDOW_NORMAL);
	//cv::imshow(source_window, dst);
	//cv::waitKey(10);
    }
    return Results;
}
}
