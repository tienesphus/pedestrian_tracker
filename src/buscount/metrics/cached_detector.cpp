#include <utility>

#include "cached_detector.hpp"

#include <opencv2/core/mat.hpp>

const Detection DUMMY_DETECTION(cv::Rect2f(0.0f, 0.0f, 1.0f, 1.0f), 0.0f);

CacheDetections::CacheDetections(const std::string& location, std::string tag):
        db(nullptr), tag(std::move(tag)) {
    if (sqlite3_open(location.c_str(), &db) != SQLITE_OK) {
        throw std::logic_error("Cannot open database");
    }
}

CacheDetections::~CacheDetections() {
    if (db) {
        sqlite3_close(db);
    }
}


nonstd::optional<Detections> CacheDetections::fetch(int frame)
{
    std::vector<Detection> detections;

    if (!exec(db, "SELECT x, y, w, h, c FROM Detections WHERE frame = '" + std::to_string(frame) + "' AND tag = '" + tag + "'",
              [&](int, char** argv, char**) -> int {
                  double x = std::stod(argv[0]);
                  double y = std::stod(argv[1]);
                  double w = std::stod(argv[2]);
                  double h = std::stod(argv[3]);
                  double c = std::stod(argv[4]);

                  detections.emplace_back(cv::Rect2d(x, y, w, h), c);
                  return 0;
              })) {
        throw std::logic_error("Cannot select from detections");
    }

    if (detections.empty()) {
        // No detections means no data was present. So state we found nothing
        return nonstd::nullopt;
    } else {
        // Some detections present, so detection has run. Check if we are just getting the dummy data detection.
        if (detections.size() == 1 && detections[0] == DUMMY_DETECTION) {
            return Detections(std::vector<Detection>());
        } else {
            return Detections(detections);
        }
    }
}

void CacheDetections::clear(int frame)
{
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM Detections WHERE frame = ? AND tag = ?", -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "Cannot delete data: "<< sqlite3_errmsg(db) << std::endl;
        throw std::logic_error("Cannot delete features");
    }

    sqlite3_bind_int(stmt, 1, frame);
    sqlite3_bind_text(stmt, 2, tag.c_str(), -1, nullptr);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::logic_error("Cannot delete detections");
    }

}

void CacheDetections::clear()
{
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM Detections WHERE tag = ?", -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "Cannot delete data: "<< sqlite3_errmsg(db) << std::endl;
        throw std::logic_error("Cannot delete features");
    }

    sqlite3_bind_text(stmt, 1, tag.c_str(), -1, nullptr);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::logic_error("Cannot delete detections");
    }
}

void CacheDetections::store(const Detection& d, int frame)
{
    if (!exec(db, std::string("INSERT INTO Detections (frame, tag, x, y, w, h, c) VALUES (")
                  + "'" + std::to_string(frame) + "', "
                  + "'" + tag + "', "
                  + "'" + std::to_string(d.box.x) + "', "
                  + "'" + std::to_string(d.box.y) + "', "
                  + "'" + std::to_string(d.box.width) + "', "
                  + "'" + std::to_string(d.box.height) + "', "
                  + "'" + std::to_string(d.confidence) + "')")) {
        throw std::logic_error("Cannot insert into detections");
    }
}

void CacheDetections::store(const Detections& detections, int frame) {
    // Delete any existing detections
    clear(frame);

    // Add the new detections
    bool any_stored = false;
    for (const Detection &d : detections.get_detections()) {
        store(d, frame);
        any_stored = true;
    }

    if (!any_stored) {
        // Insert a dummy detection so that there is always at least one detection
        store(DUMMY_DETECTION, frame);
    }
}





CachedDetectorWriter::CachedDetectorWriter(CacheDetections& cache, Detector& base):
    base(base), cache(cache), frame_no(0)
{}

CachedDetectorWriter::~CachedDetectorWriter() = default;

Detections CachedDetectorWriter::process(const cv::Mat &frame) {
    auto detections = base.process(frame);
    cache.store(detections, frame_no++);
    return detections;
}

CachedDetectorReader::CachedDetectorReader(CacheDetections& cache, float conf):
        cache(cache), conf(conf)
{}

CachedDetectorReader::~CachedDetectorReader() = default;

Detections CachedDetectorReader::process(const cv::Mat &frame) {
    u_int32_t frame_no = frame.at<uint32_t>(0, 0);
    auto cached_detections = cache.fetch(frame_no);
    if (cached_detections) {
        std::vector<Detection> detections = cached_detections->get_detections();
        std::vector<Detection> filtered;
        std::copy_if(detections.begin(), detections.end(), std::back_inserter(filtered),
                [this](const Detection& d) -> bool {
                    return d.confidence >= this->conf;
                });
        return Detections(filtered);
    } else {
        throw std::logic_error("Haven't run detection code yet for frame " + std::to_string(frame_no));
    }
}