#ifndef BUS_COUNT_CACHED_FEATURES_HPP
#define BUS_COUNT_CACHED_FEATURES_HPP

#include <sqlite3.h>
#include <optional.hpp>
#include "../tracking/tracker_component.hpp"
#include "../tracking/feature_affinity.hpp"
#include <mutex>

class FeatureCache {
public:
    FeatureCache(const std::string& location, const std::string& tag);

    ~FeatureCache();

    void clear();

    void clear(int frame_no, const Detection& d);

    void store(const FeatureData& data, int frame_no, const Detection& d);

    nonstd::optional<FeatureData> fetch(int frame_no, const Detection& d) const;

    void setTag(const std::string& tag);

private:
    sqlite3* db;
    std::string tag;
    std::map<int, std::map<std::tuple<int, int, int, int>, std::vector<float>>> feature_lookup; // map<frame, map<(x, y, w, h), FeatureData>>
    std::mutex lookup_lock;
};

class CachedFeatures: public Affinity<FeatureData> {
public:
    /**
     * Creates a cache for the given feature affinity
     * @param features the affinity to cache things for
     * @param cache somewhere to store things
     */
    CachedFeatures(const FeatureAffinity& features, FeatureCache& cache);

    /**
     * Creates an afffinity from the cache. If a value does not exist in the cache, the program will crash
     * @param cache the cache to access
     */
    CachedFeatures(FeatureCache& cache);

    std::unique_ptr<FeatureData> init(const Detection &d, const cv::Mat &frame, int frame_no) const override;

    float affinity(const FeatureData &detectionData, const FeatureData &trackData) const override;

    void merge(const FeatureData &detectionData, FeatureData &trackData) const override;

private:
    const FeatureAffinity* features;
    FeatureCache& cache;
};


#endif //BUS_COUNT_CACHED_FEATURES_HPP
