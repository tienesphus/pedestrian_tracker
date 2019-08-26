#ifndef BUS_COUNT_CACHED_DETECTOR_HPP
#define BUS_COUNT_CACHED_DETECTOR_HPP

#include "../detection/detector.hpp"

#include <sql_helper.hpp>

#include <sqlite3.h>
#include <optional.hpp>
#include <iostream>


class CacheDetections {
public:
    CacheDetections(const std::string& location, std::string tag);

    ~CacheDetections();

    nonstd::optional<Detections> fetch(int frame);

    void store(const Detections& detections, int frame);

    void clear();

    void clear(int frame);

private:
    sqlite3* db;
    std::string tag;
    void store(const Detection& d, int frame);
};

class CachedDetectorWriter: public Detector {
public:
    /**
     * Constructs a detector from the given NetConfig
     */
    CachedDetectorWriter(CacheDetections& cache, Detector& base);
    ~CachedDetectorWriter() override;

    Detections process(const cv::Mat &frame) override;

private:
    // disallow copying
    CachedDetectorWriter(const CachedDetectorWriter&);
    CachedDetectorWriter& operator=(const CachedDetectorWriter&);

    Detector& base;
    CacheDetections& cache;
    uint32_t frame_no;
};

class CachedDetectorReader: public Detector {
public:
    /**
     * Constructs a detector from the given NetConfig
     */
    explicit CachedDetectorReader(CacheDetections& cache, float conf);
    ~CachedDetectorReader() override;

    Detections process(const cv::Mat &frame) override;

private:
    // disallow copying
    CachedDetectorReader(const CachedDetectorReader&);
    CachedDetectorReader& operator=(const CachedDetectorReader&);

    CacheDetections& cache;
    float conf;
};


#endif //BUS_COUNT_CACHED_DETECTOR_HPP
