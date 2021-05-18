#ifndef BUS_COUNT_FEATURE_AFFINITY_HPP
#define BUS_COUNT_FEATURE_AFFINITY_HPP

#include "tracker_component.hpp"
#include <inference_engine.hpp>


/**
 * Data for FeatureAffinity. Just holds the feature vector
 */
class FeatureData: public TrackData
{
public:
    std::vector<float> features;

public:
    explicit FeatureData(std::vector<float> features);

    ~FeatureData() override = default;
};

/**
 * An affinity that based on a 're-identification' network
 */
class FeatureAffinity: public Affinity<FeatureData> {
public:

    struct NetConfig {
        std::string meta;       // path to the meta file (.xml)
        std::string model;      // path to the model file (.bin)
        cv::Size size;          // Size of input layer
    };

    FeatureAffinity(const NetConfig& net, InferenceEngine::Core &plugin);

    ~FeatureAffinity() override;

    std::unique_ptr<FeatureData> init(const Detection& d, const cv::Mat& frame, int frame_no) const override;

    float affinity(const FeatureData &detectionData, const FeatureData &trackData) const override;

    void merge(const FeatureData& detectionData, FeatureData& trackData) const override;

private:

    InferenceEngine::ExecutableNetwork network;
    std::string inputName, outputName;

    std::vector<float> identify(const cv::Mat &person) const;

};


#endif //BUS_COUNT_FEATURE_AFFINITY_HPP
