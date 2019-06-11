
#include "detector_opencv.hpp"

#include <iostream>


OpenCVDetector::OpenCVDetector(const NetConfigOpenCV &config, cv::Size size):
        Detector(config.thresh, config.clazz, std::move(size)),
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

cv::Mat OpenCVDetector::run(const cv::Mat &frame) {
    cv::Mat blob = cv::dnn::blobFromImage(frame, config.scale, config.networkSize, this->config.mean);

    // TODO how much locking do we really need?
    std::lock_guard<std::mutex> scoped(lock);
    this->net.setInput(blob);
    return this->net.forward();
}