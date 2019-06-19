#ifndef DETECTION_OPENVINO_H
#define DETECTION_OPENVINO_H

#include <inference_engine.hpp>

#include "detector.hpp"

class DetectorOpenVino: public Detector {

public:

    struct NetConfig {
        float thresh;           // confidence to threshold positive detections at (between 0-1)
        int clazz;              // class number of people (normally 0, 1, or 15)
        std::string meta;       // path to the meta file (.xml)
        std::string model;      // path to the model file (.bin)
    };

    /**
     * Constructs a detector from the given NetConfig
     */
    explicit DetectorOpenVino(const NetConfig &config);
    virtual ~DetectorOpenVino();

    Detections process(const cv::Mat &frame) override;

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
