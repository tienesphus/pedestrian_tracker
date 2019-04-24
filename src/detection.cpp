
#include "detection.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/cvstd.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>

//  ----------- DETECTION ---------------

Detection::Detection(const cv::Rect2d& box, float confidence):
    box(box), confidence(confidence)
{}

void Detection::draw(cv::Mat& display) const
{
    cv::rectangle(display, this->box, cv::Scalar(0, 0, this->confidence*255), 3);
}


//  ----------- DETECTIONS ---------------

Detections::Detections(const cv::Ptr<cv::Mat>& frame, const std::vector<Detection> &detections):
    frame(frame), detections(detections)
{
}

cv::Ptr<cv::Mat> Detections::get_frame() const
{
    return frame;
}

std::vector<Detection> Detections::get_detections() const
{
    return this->detections;
}

void Detections::draw(cv::Mat& display) const
{
    for (Detection d : this->detections)
        d.draw(display);
}


//  ----------- DETECTOR ---------------

Detector::Detector(const NetConfigIR &config):
        clazz(config.clazz), thresh(config.thresh), size(config.size), 
        scale(config.scale), mean(config.mean)
{
    // Use the OpenVino (must set prefered backend to Inference Engine. Works on Pi with NCS) 
    this->net = cv::dnn::readNetFromModelOptimizer(config.xml, config.bin);
    
    // use the optimised OpenVino implementation
    this->net.setPreferableBackend(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE);
    this->net.setPreferableTarget(cv::dnn::DNN_TARGET_MYRIAD);
}


// Takes an input picture and converts it to a blob ready for 
// passing into a neural network
void preprocess(const cv::Mat &image, cv::Mat &result_blob, const cv::Size &size, const cv::Scalar& mean, float scale)
{
    cv::Mat resized;
    std::cout << "  Preprocessing image" << std::endl;
    cv::resize(image, resized, size);
    cv::dnn::blobFromImage(resized, result_blob, scale, size, mean);
    //result.convertTo(result, CV_32F, 1/127.5, -1);
}

std::vector<Detection> postprocess(cv::Mat result, int w, int h, float thresh, int clazz)
{
    std::cout << "  Interpretting Results" << std::endl;
    
    // result is of size [nimages, nchannels, a, b]
    // nimages = 1 (as only one image at a time)
    // nchannels = 1 (for RGB/BW?, not sure why we only have one, or why there would be a channel dimension)
    // a is number of predictions (depends on archutecture)
    // b is 7 floats of data. The order is:
    //  0 - ? (always 0?)
    //  1 - class index
    //  2 - confidennce
    //  3 - x1 (co-ords are scaled between 0 - 1)
    //  4 - y1
    //  5 - x2
    //  6 - y2
    
    // Therefore, we discard first two dimensions to only have the data
    cv::Mat detections(result.size[2], result.size[3], CV_32F, result.ptr<float>());

    std::vector<Detection> results;
    for (int i = 0; i < detections.size[0]; i++) {
        float confidence = detections.at<float>(i, 2);
        if (confidence > thresh) {
            int id = int(detections.at<float>(i, 1));
            int x1 = int(detections.at<float>(i, 3) * w);
            int y1 = int(detections.at<float>(i, 4) * h);
            int x2 = int(detections.at<float>(i, 5) * w);
            int y2 = int(detections.at<float>(i, 6) * h);
            cv::Rect2d r(cv::Point2d(x1, y1), cv::Point2d(x2, y2));
            
            std::cout << "    Found: " << id << "(" << confidence << "%) - " << r << std::endl;
            if (id == clazz)
                results.push_back(Detection(r, confidence));
        }
    }

    return results;
}

cv::Ptr<Detections> Detector::process(const cv::Ptr<cv::Mat> &frame) {
    // TODO move pre/post processing to a seperate task
    
    // preprocess
    cv::Mat preprocessed_blob;
    preprocess(*frame, preprocessed_blob, this->size, this->mean, this->scale);

    // pass the network
    net.setInput(preprocessed_blob);
    cv::Mat results = net.forward();
    
    // post_process
    std::vector<Detection> detects = postprocess(results, frame->cols, frame->rows, this->thresh, this->clazz);
    
    return cv::Ptr<Detections>(new Detections(frame, detects));
}
