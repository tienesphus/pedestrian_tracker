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

#include <detection.hpp>

using namespace std;
using namespace cv;

#define NETWORK_INPUT_SIZE  300
#define NETWORK_OUTPUT_SIZE 707
#define PERSON_CLASS 15

bool check_status(bool success, NCSWrapper &NCS)
{
    if(!success)
    {
      if (NCS.ncsCode == MVNC_MYRIAD_ERROR)
      {
        char* err;
        unsigned int len;
        mvncGetGraphOption(NCS.ncsGraph, MVNC_DEBUG_INFO, (void*)&err, &len);
        cout<<string(err, len)<<endl;
      }
      return false;
    }
    return true;
}

void preprocess(Mat &image, Mat &result)
{
    Mat resized;
    cout << "Preprocessing image" << endl;
    cv::resize(image, result, Size(NETWORK_INPUT_SIZE, NETWORK_INPUT_SIZE));
    result.convertTo(result, CV_32F, 1/127.5, -1);
}

vector<Detection> postprocess(float* result, int width, int height)
{
    vector<Detection> detections;
    cout << "Interpretting Results" << endl;
    int num = result[0];
    float thresh = 0.4;
    
    for (int i=1; i<num+1; i++)
    {
      float score = result[i*7+2];
      int cls = int(result[i*7+1]);
      if (score>thresh and cls == PERSON_CLASS)
      {
        Rect box = Rect(result[i*7+3]*width, result[i*7+4]*height,
                            (result[i*7+5]-result[i*7+3])*width, 
                            (result[i*7+6]-result[i*7+4])*height);
        detections.push_back(Detection{box, cls, score});
      }
    }
    
    return detections;
}

NCSWrapper NCS_create()
{
    NCSWrapper NCS(NETWORK_INPUT_SIZE*NETWORK_INPUT_SIZE*3, NETWORK_OUTPUT_SIZE);
    if (!NCS.load_file("./models/default/mobilenet_graph"))
      cout << "FAILED TO LOAD GRAPH!!!!" << endl;
    return NCS;    
}


// Begins an inferences on an image and gets the results from the last image
// Note that the order of execution here allows parrallel as the inference is being
// run while this method is not being executed.
vector<Detection> NCS_swap_image(Mat &image, NCSWrapper &NCS)
{
    static bool first_call = true;
    static Mat preprocessed;
    
    if (first_call) {
      // We always need some data already loaded, so we use the first frame as a dummy
      cout << "Dummy preprocess" << endl;
      preprocess(image, preprocessed);
      cout << "Dummy load tensor" << endl;
      if(!check_status(NCS.load_tensor_nowait((float*)preprocessed.data), NCS))
        return {};
    }
    
    float* results;
    
    // Get Previous results back
    cout << "Getting Results" << endl;
    if(!check_status(NCS.get_result(results), NCS))
      return {};
    
    //load data to NCS
    cout << "Loading Tensor" << endl;
    if(!check_status(NCS.load_tensor_nowait((float*)preprocessed.data), NCS))
        return {};
    
    // preprocess the image for the next iteration
    cout << "Preprocessing Tensor" << endl;
    preprocess(image, preprocessed);
        
    // Calculate the results
    if (first_call) {
        // filter out the dummy image we put into it
        first_call = false;
        return {};
    } else {
        // actual results
        return postprocess(results, image.cols, image.rows);
    }
    
    
}

