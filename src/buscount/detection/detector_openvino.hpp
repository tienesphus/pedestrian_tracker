#ifndef DETECTION_OPENVINO_H
#define DETECTION_OPENVINO_H

#include "detector.hpp"

#include <inference_engine.hpp>

/**
 * Runs detections using the OpenVino backend.
 */
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
    DetectorOpenVino(const NetConfig &config, InferenceEngine::InferencePlugin &plugin);

    ~DetectorOpenVino() override;

    Detections process(const cv::Mat &frame, int frame_no) override;

private:
    // disallow copying
    DetectorOpenVino(const DetectorOpenVino&);
    DetectorOpenVino& operator=(const DetectorOpenVino&);

    NetConfig config;
    InferenceEngine::ExecutableNetwork network;
    std::string inputName, outputName;
    size_t objectSize;
    size_t maxProposalCount;
    std::mutex ncs_lock;
};

#endif
