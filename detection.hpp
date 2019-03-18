#ifndef DETECTION_HEADER
#define DETECTION_HEADER

#include <stdio.h>
#include <iostream>
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/tracking.hpp"
#include "opencv2/dnn.hpp"

//Neural compute stick
#include <mvnc.h>
#include <./wrapper/ncs_wrapper.hpp>

using namespace std;
using namespace cv;

struct Detection {
  Rect2d box;
  int clazz;
  float confidence;
};

NCSWrapper NCS_create();


// Begins an inferences on an image and gets the results from the last image
vector<Detection> NCS_swap_image(Mat &image, NCSWrapper &NCS);

#endif
