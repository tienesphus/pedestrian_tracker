#ifndef BUS_COUNT_CACHED_FEATURES_HPP
#define BUS_COUNT_CACHED_FEATURES_HPP

#include <sqlite3.h>
#include <optional.hpp>
#include "../tracking/tracker_component.hpp"
#include "../tracking/feature_affinity.hpp"

class FeatureCache {
public:
    FeatureCache(const std::string& location, std::string tag);

    ~FeatureCache();

    void clear();

    void clear(int frame_no, const Detection& d);

    void store(const FeatureData& data, int frame_no, const Detection& d);

    nonstd::optional<FeatureData> fetch(int frame_no, const Detection& d) const;

private:
    sqlite3* db;
    std::string tag;
};

class CachedFeatures: public Affinity<FeatureData> {
public:
    CachedFeatures(const FeatureAffinity& features, FeatureCache& cache);

    std::unique_ptr<FeatureData> init(const Detection &d, const cv::Mat &frame, int frame_no) const override;

    float affinity(const FeatureData &detectionData, const FeatureData &trackData) const override;

    void merge(const FeatureData &detectionData, FeatureData &trackData) const override;

private:
    const FeatureAffinity& features;
    FeatureCache& cache;
};


#endif //BUS_COUNT_CACHED_FEATURES_HPP
