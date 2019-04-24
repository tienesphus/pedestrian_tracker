
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
        clazz(config.clazz), thresh(config.thresh), networkSize(config.networkSize), 
        scale(config.scale), mean(config.mean)
{
    // Use the OpenVino (must set prefered backend to Inference Engine. Works on Pi with NCS) 
    this->net = cv::dnn::readNetFromModelOptimizer(config.xml, config.bin);
    
    // use the optimised OpenVino implementation
    this->net.setPreferableBackend(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE);
    this->net.setPreferableTarget(cv::dnn::DNN_TARGET_MYRIAD);
}


void Detector::pre_process(const cv::Mat &image)
{
    cv::Mat blob;
    cv::dnn::blobFromImage(image, blob, this->scale, this->networkSize, this->mean);
    //result.convertTo(result, CV_32F, 1/127.5, -1);
    this->net.setInput(blob);
}

cv::Ptr<cv::Mat> Detector::process() {
    // pass the network
    cv::Ptr<cv::Mat> results(new cv::Mat());
    net.forward(*results);
    return results;
}

cv::Ptr<Detections> Detector::post_process(const cv::Ptr<cv::Mat>& original, cv::Mat& data) const
{   
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
    cv::Mat detections(data.size[2], data.size[3], CV_32F, data.ptr<float>());

    std::vector<Detection> results;
    int w = original->cols;
    int h = original->rows;
    
    for (int i = 0; i < detections.size[0]; i++) {
        float confidence = detections.at<float>(i, 2);
        if (confidence > this->thresh) {
            int id = int(detections.at<float>(i, 1));
            int x1 = int(detections.at<float>(i, 3) * w);
            int y1 = int(detections.at<float>(i, 4) * h);
            int x2 = int(detections.at<float>(i, 5) * w);
            int y2 = int(detections.at<float>(i, 6) * h);
            cv::Rect2d r(cv::Point2d(x1, y1), cv::Point2d(x2, y2));
            
            std::cout << "    Found: " << id << "(" << confidence << "%) - " << r << std::endl;
            if (id == this->clazz)
                results.push_back(Detection(r, confidence));
        }
    }
    
    return cv::Ptr<Detections>(new Detections(original, results));
}
