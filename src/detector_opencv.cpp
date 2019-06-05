
#include "detector_opencv.hpp"

#include <iostream>


OpenCVDetector::OpenCVDetector(const NetConfigOpenCV &config):
        Detector(config.thresh, config.clazz),
        net(cv::dnn::readNet(config.model, config.meta))
{
    std::cout << "Init detector" << std::endl;

    /**
     * IMPORTANT NOTE:
     *  There seems to be a strange bug with Pi/OpenVino that requires
     *  the network to never be copied. If the network is copied, calling
     *  net.setPreferableTarget will cause a segfault. Apparently this
     *  only occurs on the Pi. By extension, this means Detector should
     *  never be copied. Perhaps replacing Net with Ptr<Net> is an option.
     */

    net.setPreferableBackend(config.preferableBackend);
    net.setPreferableTarget(config.preferableTarget);
}

cv::Mat OpenCVDetector::run(const cv::Mat &frame) {
    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, config.scale, config.networkSize, this->config.mean);

    // pass the network
    cv::Mat result;

    // TODO how much locking do we really need?
    std::lock_guard<std::mutex> scoped(lock);
    this->net.setInput(blob);
    return this->net.forward();
}