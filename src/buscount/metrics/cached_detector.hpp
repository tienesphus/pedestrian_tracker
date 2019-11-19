#ifndef BUS_COUNT_CACHED_DETECTOR_HPP
#define BUS_COUNT_CACHED_DETECTOR_HPP

#include "../detection/detector.hpp"

#include <sql_helper.hpp>

#include <sqlite3.h>
#include <optional.hpp>
#include <map>


class DetectionCache {
public:
    DetectionCache(const std::string& location, const std::string& tag);

    ~DetectionCache();

    nonstd::optional<Detections> fetch(int frame);

    void store(const Detections& detections, int frame);

    void clear();

    void clear(int frame);

    void setTag(const std::string& new_tag);

private:
    sqlite3* db;
    std::string tag;
    void store(const Detection& d, int frame);
    std::map<int, Detections> detections_lookup;
    std::mutex lookup_lock;
};

class CachedDetector: public Detector {
public:
    /**
     * Constructs a cache around the given detector
     * The confidence given to the NetConfig in base will be used to filter what is stored in the cache (and thus all
     * future runs. It should be set low (~0.1)).
     * The confidence given to this constructor will be used to filter detections for this run only (it should be set
     * high (~0.6)).
     */
    CachedDetector(DetectionCache& cache, Detector& base, float conf);

    /**
     * Contructs a detector that reads from the given cache. If a frame does not exist in the cache, an exeption is thrown
     * @param cache where to read from
     * @param conf the confidence to filter frames on
     */
    CachedDetector(DetectionCache& cache, float conf);

    ~CachedDetector() override;

    Detections process(const cv::Mat &frame, int frame_no) override;

private:
    // disallow copying
    CachedDetector(const CachedDetector&);
    CachedDetector& operator=(const CachedDetector&);

    Detector* base;
    DetectionCache& cache;
    float conf;
};


#endif //BUS_COUNT_CACHED_DETECTOR_HPP
