#ifndef DETECTION_HEADER
#define DETECTION_HEADER

#include <stdio.h>
#include "opencv2/dnn.hpp"

using namespace std;
using namespace cv;
using namespace cv::dnn;

struct Detection {
  Rect2d box;
  int clazz;
  float confidence;
};

Net create_net();


// Begins an inferences on an image and gets the results from the last image
vector<Detection> swap_image(Mat &image, Net &net);
#endif
