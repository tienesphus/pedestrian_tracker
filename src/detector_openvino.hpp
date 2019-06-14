#ifndef DETECTION_OPENVINO_H
#define DETECTION_OPENVINO_H

#include <inference_engine.hpp>

#include "detector.hpp"

class DetectorOpenVino: public Detector {

public:
    /**
     * Constructs a detector from the given NetConfig
     */
    explicit DetectorOpenVino(const NetConfig &config);

    Detections post_process(const cv::Mat &data) const override;

protected:
    cv::Mat run(const cv::Mat &frame) override;

private:
    // disallow copying
    DetectorOpenVino(const DetectorOpenVino&);
    DetectorOpenVino& operator=(const DetectorOpenVino&);

    NetConfig config;
    InferenceEngine::ExecutableNetwork network;
    std::string inputName, outputName;
    size_t objectSize;
    size_t maxProposalCount;
};

#endif
