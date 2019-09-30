
#include "detector_opencv.hpp"
#include "../cv_utils.hpp"

#include <spdlog/spdlog.h>

DetectorOpenCV::DetectorOpenCV(const NetConfig &config) :
        config(config),
        net(cv::dnn::readNet(config.model, config.meta))
{
    /**
     * IMPORTANT NOTE:
     *  There seems to be a strange bug with Pi/OpenVino that requires
     *  the network to never be copied. If the network is copied, calling
     *  net.setPreferableTarget will cause a segfault. Apparently this
     *  only occurs on the Pi. By extension, this means Detector should
     *  never be copied. Perhaps replacing Net with Ptr<Net> is an option.
     *
     *  It is for this reason that the copy constructor on this class is
     *  made private (so it cannot be called)
     */

    net.setPreferableBackend(config.preferableBackend);
    net.setPreferableTarget(config.preferableTarget);
}

DetectorOpenCV::~DetectorOpenCV() {}

Detections DetectorOpenCV::process(const cv::Mat &frame, int) {
    cv::Mat blob = cv::dnn::blobFromImage(frame, config.scale, config.networkSize, this->config.mean);

    cv::Mat result;
    {
        std::lock_guard<std::mutex> scoped(lock);
        this->net.setInput(blob);
        result = this->net.forward();
    }
    return static_post_process(result, config.clazz, config.thresh);
}

Detections static_post_process(const cv::Mat &data, int clazz, float thresh)
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

    for (int i = 0; i < detections.size[0]; i++) {
        float confidence = detections.at<float>(i, 2);
        if (confidence > thresh) {
            int id = int(detections.at<float>(i, 1));
            float x1 = detections.at<float>(i, 3);
            float y1 = detections.at<float>(i, 4);
            float x2 = detections.at<float>(i, 5);
            float y2 = detections.at<float>(i, 6);
            cv::Rect2d r(cv::Point2d(x1, y1), cv::Point2d(x2, y2));

            spdlog::debug( "    Found: {} ({}%) - ({},{}),({}x{})", id, confidence, r.x, r.y, r.width, r.height);
            if (id == clazz)
                results.emplace_back(r, confidence);
        }
    }



    return Detections(results);
}
