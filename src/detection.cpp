#include <stdio.h>
#include <iostream>
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/tracking.hpp"
#include "opencv2/dnn.hpp"

#include <detection.hpp>

using namespace std;
using namespace cv;
using namespace cv::dnn;

#define NETWORK_INPUT_SIZE (Size(300, 300))
#define NETWORK_OUTPUT_SIZE 707
#define NETWORK_SCALE (2/255.0)
#define NETWORK_MEAN (Scalar(127.5, 127.5, 127.5))
#define PERSON_CLASS 15
#define MODEL_PROTO "./models/default/patched_MobileNetSSD_deploy.prototxt"
#define MODEL_BIN "./models/default/MobileNetSSD_deploy.caffemodel"

// Takes an input picture and converts it to a blob ready for 
// passing into a neural network
void preprocess(Mat &image, Mat &result_blob)
{
    Mat resized;
    cout << "Preprocessing image" << endl;
    cv::resize(image, resized, NETWORK_INPUT_SIZE);
    blobFromImage(resized, result_blob, NETWORK_SCALE, NETWORK_INPUT_SIZE, NETWORK_MEAN);
    //result.convertTo(result, CV_32F, 1/127.5, -1);
}

vector<Detection> postprocess(Mat result, int w, int h)
{
    cout << "Interpretting Results" << endl;
    
    // result is of size [nimages, nchannels, a, b]
    // nimages = 1 (as only one image at a time)
    // nchannels = 1 (for RGB/BW, not sure why we only have one, or why there would be a channel dimension)
    // a is number of predictions (depends on archutecture)
    // b is 7 floats of data. The order is:
    //  0 - ? (always 0?)
    //  1 - class index
    //  2 - confidennce
    //  3 - x1 (scaled between 0 - 1)
    //  4 - y1
    //  5 - x2
    //  6 - y2
    
    // Therefore, we discard first two dimensions to only have the data
    Mat detections(result.size[2], result.size[3], CV_32F, result.ptr<float>());

    //cout << "  Detections size: " << detections.size << " (" << detections.size[0] << ")" << endl;
    /*for (int i = 0; i < detections.size[0]; i++) {
    cout << "  ";
    for (int j = 0; j < detections.size[1]; j++) {
      cout << detections.at<float>(i, j) << " ";
    }
    cout << endl;
    }*/

    cout << "  Reading detection" << endl;
    //cout << result.size << endl;

    std::vector<Detection> results;
    for (int i = 0; i < detections.size[0]; i++) {
        float confidence = detections.at<float>(i, 2);
        if (confidence > 0.5) {
            int id = int(detections.at<float>(i, 1));
            int x1 = int(detections.at<float>(i, 3) * w);
            int y1 = int(detections.at<float>(i, 4) * h);
            int x2 = int(detections.at<float>(i, 5) * w);
            int y2 = int(detections.at<float>(i, 6) * h);
            Rect2d r = Rect2d(Point2d(x1, y1), Point2d(x2, y2));

            cout << "    Found: " << id << "(" << confidence << "%) - " << r << endl;
            results.push_back(Detection{r, id, confidence});
        }
    }

    return results;
}

Net create_net()
{
    Net net = readNetFromCaffe(MODEL_PROTO, MODEL_BIN);
    
    // a generic implementation that will probably work for everyone
    net.setPreferableBackend(DNN_BACKEND_OPENCV);
    
    // Alternatively, use the optimised OpenVino implementation 
    //net.setPreferableBackend(DNN_BACKEND_INFERENCE_ENGINE);
    //net.setPreferableTarget(DNN_TARGET_MYRIAD); // change to match your system
    return net;    
}


// Begins an inferences on an image and gets the results from the last image
// Note that the order of execution here allows parrallel as the inference is being
// run while this method is not being executed.
vector<Detection> swap_image(Mat &image, Net &net)
{
    // IMPORTANT NOTE:
    // The API of this method is designed to allow the passing of the image
    // to be done asyncronously. Currently, it is syncronous. Moving to
    // asyncronous will approx double the fps since then we can fully utilise
    // both the CPU and the Movidius. For the sydney trial, I did have it running
    // asyncronously, however, I removed it when converting from the depricated
    // NCS sdk to OpenVino (just because it was simpler to do).
    
    
    static bool first_call = true;
    static Mat preprocessed_blob;
    
    if (first_call) {
      // We always need some data already loaded, so we use the first frame as a dummy
      cout << "Dummy preprocess" << endl;
      preprocess(image, preprocessed_blob);
    }
    
    // pass the network
    net.setInput(preprocessed_blob);
    Mat results = net.forward();
    
    // preprocess the image for the next iteration
    cout << "Preprocessing Tensor" << endl;
    preprocess(image, preprocessed_blob);
        
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

