#ifndef BUS_COUNT_FEATURE_AFFINITY_HPP
#define BUS_COUNT_FEATURE_AFFINITY_HPP

#include "tracker_component.hpp"
#include <inference_engine.hpp>

class FeatureData: public TrackData
{
    friend class FeatureAffinity;

    std::vector<float> features;

public:
    explicit FeatureData(std::vector<float> features);

    ~FeatureData() override = default;
};

class FeatureAffinity: public Affinity<FeatureData> {
public:

    struct NetConfig {
        std::string meta;       // path to the meta file (.xml)
        std::string model;      // path to the model file (.bin)
        cv::Size size;          // Size of input layer
        float thresh;           // confidence to threshold positive detections at (between 0-1)
    };

    FeatureAffinity(const NetConfig& net, InferenceEngine::InferencePlugin &plugin);

    ~FeatureAffinity() override;

    std::unique_ptr<FeatureData> init(const Detection& d, const cv::Mat& frame) const override;

    float affinity(const FeatureData &detectionData, const FeatureData &trackData) const override;

    void merge(const FeatureData& detectionData, FeatureData& trackData) const override;

    void draw(const FeatureData& data, cv::Mat &img) const override;

private:

    NetConfig netConfig;
    InferenceEngine::ExecutableNetwork network;
    std::string inputName, outputName;

    std::vector<float> identify(const cv::Mat &person) const;

};


#endif //BUS_COUNT_FEATURE_AFFINITY_HPP
