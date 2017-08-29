#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include "stdio.h"

int main(int argc, char** argv){
  //cv::VideoCapture cap_down(0);
  cv::VideoCapture cap_forward(0);
  //cv::Mat image_down;
  cv::Mat image_forward;
  if(/*!cap_down.isOpened() ||*/ !cap_forward.isOpened()){
    printf("cap open failed...\n");
    return -1;
  }
  //cap_down.read(image_down);
  //cap_down.read(image_down);
  //cap_down.read(image_down);
  cap_forward.read(image_forward);
  cap_forward.read(image_forward);
  cap_forward.read(image_forward);
  //cv::namedWindow("win_down", CV_WINDOW_NORMAL);
  cv::namedWindow("win_forward", CV_WINDOW_NORMAL);
  while(/*cap_down.isOpened() && */cap_forward.isOpened()){
   //cap_down.read(image_down);
    cap_forward.read(image_forward);
    //cv::imshow("win_down", image_down);
    cv::imshow("win_forward", image_forward);
    printf("fuck\n");
    if(' ' == cv::waitKey(3)){
      exit(0);
    }
  }
   
  return 0;
}
