#include "detector.hpp"
#include <opencv2/dnn.hpp>
#include <iostream>

Detector::Detector(float thresh, int clazz, cv::Size size)
    :thresh(thresh), clazz(clazz), input_size(std::move(size))
{
}

// TODO can Detector::start_async be merged with Detector::wait_async and suffer threading penalty?

std::shared_future<cv::Mat> Detector::start_async(const cv::Mat &frame)
{
    return std::async(
            [&]() { return this->run(frame); }
    );
}

cv::Mat Detector::wait_async(const std::shared_future<cv::Mat> &request) const
{
    return request.get();
}

Detections Detector::post_process(const cv::Mat &data) const
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
    cv::Mat detections(data.size[2], data.size[3], CV_32F, (void*)data.ptr<float>());

    std::vector<Detection> results;
    int w = input_size.width;
    int h = input_size.height;

    for (int i = 0; i < detections.size[0]; i++) {
        float confidence = detections.at<float>(i, 2);
        if (confidence > this->thresh) {
            int id = int(detections.at<float>(i, 1));
            int x1 = int(detections.at<float>(i, 3) * w);
            int y1 = int(detections.at<float>(i, 4) * h);
            int x2 = int(detections.at<float>(i, 5) * w);
            int y2 = int(detections.at<float>(i, 6) * h);
            cv::Rect r(cv::Point(x1, y1), cv::Point(x2, y2));

            std::cout << "    Found: " << id << "(" << confidence << "%) - " << r << std::endl;
            if (id == this->clazz)
                results.emplace_back(r, confidence);
        }
    }

    return Detections(results);
}

Detections Detector::process(const cv::Mat &frame) {
    return post_process(run(frame));
}