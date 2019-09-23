#include <utility>

#include "cached_detector.hpp"

#include <opencv2/core/mat.hpp>

// The dummy detection is used to differentiate between a frame that has no detections
// and a frame that has not been processed yet
const Detection DUMMY_DETECTION(cv::Rect2f(0.0f, 0.0f, 1.0f, 1.0f), 0.0f);

DetectionCache::DetectionCache(const std::string& location, const std::string& tag):
        db(nullptr), tag(tag), detections_lookup({}) {
    if (sqlite3_open(location.c_str(), &db) != SQLITE_OK) {
        throw std::logic_error("Cannot open database");
    }

    // Reload the detection_lookup
    setTag(tag);
}

DetectionCache::~DetectionCache() {
    if (db) {
        sqlite3_close(db);
    }
}


nonstd::optional<Detections> DetectionCache::fetch(int frame)
{
    const Detections& detections = detections_lookup[frame];
    const std::vector<Detection>& list = detections.get_detections();

    if (list.empty()) {
        // No detections means no data was present. So state we found nothing
        return nonstd::nullopt;
    } else {
        // Some detections present, so detection has run. Check if we are just getting the dummy data detection.
        if (list.size() == 1 && list[0] == DUMMY_DETECTION) {
            return Detections();
        } else {
            return detections;
        }
    }
}

void DetectionCache::setTag(const std::string &new_tag)
{
    this->tag = new_tag;

    // reload the database into ram
    // Note: we load the detections into ram, otherwise, the SQL call will limit the performance to
    // approximately 150fps

    detections_lookup.clear();
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, "SELECT frame, x, y, w, h, c FROM Detections WHERE tag = ?", -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "Cannot delete data: "<< sqlite3_errmsg(db) << std::endl;
        throw std::logic_error("Cannot load detections from database");
    }
    sqlite3_bind_text(stmt, 1, new_tag.c_str(), -1, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int frame = sqlite3_column_int(stmt, 0);
        double x = sqlite3_column_double(stmt, 1);
        double y = sqlite3_column_double(stmt, 2);
        double w = sqlite3_column_double(stmt, 3);
        double h = sqlite3_column_double(stmt, 4);
        double c = sqlite3_column_double(stmt, 5);

        detections_lookup[frame].emplace_back(cv::Rect2f(x, y, w, h), c);
    }

    sqlite3_finalize(stmt);
}

void DetectionCache::clear(int frame)
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
    sqlite3_finalize(stmt);

    // update the detection cache
    detections_lookup.erase(frame);
}

void DetectionCache::clear()
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
    sqlite3_finalize(stmt);

    // update the ram cache
    detections_lookup.clear();
}

void DetectionCache::store(const Detection& d, int frame)
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

    detections_lookup[frame].push_back(d);
}

void DetectionCache::store(const Detections& detections, int frame) {
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





CachedDetector::CachedDetector(DetectionCache& cache, Detector& base, float conf):
    base(&base), cache(cache), conf(conf)
{}

CachedDetector::CachedDetector(DetectionCache& cache, float conf):
        base(nullptr), cache(cache), conf(conf)
{}

CachedDetector::~CachedDetector() = default;

Detections CachedDetector::process(const cv::Mat &frame, int frame_no) {
    auto opt_detections = cache.fetch(frame_no);
    std::vector<Detection> detections;
    if (opt_detections.has_value()) {
        detections = opt_detections->get_detections();
    } else {
        if (base) {
            auto detection_res = base->process(frame, frame_no);
            cache.store(detection_res, frame_no);
            detections = detection_res.get_detections();
        } else {
            throw std::logic_error(&"Frame not known: " [ frame_no]);
        }
    }
    std::vector<Detection> filtered;
    std::copy_if(detections.begin(), detections.end(), std::back_inserter(filtered),
                 [this](const Detection &d) -> bool {
                     return d.confidence >= this->conf;
                 });
    return Detections(filtered);
}