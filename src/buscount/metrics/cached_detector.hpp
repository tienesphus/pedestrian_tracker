#ifndef BUS_COUNT_CACHED_DETECTOR_HPP
#define BUS_COUNT_CACHED_DETECTOR_HPP

#include "../detection/detector.hpp"

#include <sql_helper.hpp>

#include <sqlite3.h>
#include <optional.hpp>
#include <iostream>


class DetectionCache {
public:
    DetectionCache(const std::string& location, std::string tag);

    ~DetectionCache();

    nonstd::optional<Detections> fetch(int frame);

    void store(const Detections& detections, int frame);

    void clear();

    void clear(int frame);

private:
    sqlite3* db;
    std::string tag;
    void store(const Detection& d, int frame);
};

class CachedDetector: public Detector {
public:
    /**
     * Constructs a detector from the given NetConfig
     */
    CachedDetector(DetectionCache& cache, Detector& base, float conf);
    ~CachedDetector() override;

    Detections process(const cv::Mat &frame, int frame_no) override;

private:
    // disallow copying
    CachedDetector(const CachedDetector&);
    CachedDetector& operator=(const CachedDetector&);

    Detector& base;
    DetectionCache& cache;
    float conf;
};


#endif //BUS_COUNT_CACHED_DETECTOR_HPP
