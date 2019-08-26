#include "cached_features.hpp"

#include <sql_helper.hpp>


FeatureCache::FeatureCache(const std::string& location, std::string tag):
        db(nullptr), tag(std::move(tag))
{
    if (sqlite3_open(location.c_str(), &db) != SQLITE_OK) {
        throw std::logic_error("Cannot open database");
    }
}

FeatureCache::~FeatureCache()
{
    if (db) {
        sqlite3_close(db);
    }
}

void FeatureCache::clear() {
    // Delete any existing features in that location
    sqlite3_stmt* delete_stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM Features WHERE tag=?", -1, &delete_stmt, nullptr) != SQLITE_OK) {
        std::cout << "Cannot delete data: "<< sqlite3_errmsg(db) << std::endl;
        throw std::logic_error("Cannot delete features");
    }
    sqlite3_bind_text(delete_stmt, 1, tag.c_str(), -1, nullptr);

    if (sqlite3_step(delete_stmt) != SQLITE_DONE) {
        throw std::logic_error("Cannot delete features");
    }
}

void FeatureCache::clear(int frame_no, const Detection& d) {
    sqlite3_stmt* delete_stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM Features WHERE tag=? AND frame=? AND x=? AND y=? AND w=? AND h=?", -1, &delete_stmt, nullptr) != SQLITE_OK) {
        std::cout << "Cannot delete data: "<< sqlite3_errmsg(db) << std::endl;
        throw std::logic_error("Cannot delete features");
    }
    sqlite3_bind_text(delete_stmt, 1, tag.c_str(), -1, nullptr);
    sqlite3_bind_int(delete_stmt, 2, frame_no);
    sqlite3_bind_int(delete_stmt, 3, static_cast<int>(std::round(d.box.x*1000000)));
    sqlite3_bind_int(delete_stmt, 4, static_cast<int>(std::round(d.box.y*1000000)));
    sqlite3_bind_int(delete_stmt, 5, static_cast<int>(std::round(d.box.width*1000000)));
    sqlite3_bind_int(delete_stmt, 6, static_cast<int>(std::round(d.box.height*1000000)));

    if (sqlite3_step(delete_stmt) != SQLITE_DONE) {
        throw std::logic_error("Cannot delete features");
    }
}

void FeatureCache::store(const FeatureData &data, int frame_no, const Detection &d) {

    // Delete any existing features in that location
    clear(frame_no, d);

    // Add new features
    std::vector<float> features = data.features;

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO Features (tag, frame, x, y, w, h, data) VALUES (?, ?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "Cannot insert data: "<< sqlite3_errmsg(db) << std::endl;
        throw std::logic_error("Cannot insert features");
    }

    if (sqlite3_bind_text(stmt, 1, tag.c_str(), -1, nullptr) != SQLITE_OK) {
        throw std::logic_error("Cannot bind data");
    }
    sqlite3_bind_int(stmt, 2, frame_no);

    sqlite3_bind_int(stmt, 3, static_cast<int>(std::round(d.box.x*1000000)));
    sqlite3_bind_int(stmt, 4, static_cast<int>(std::round(d.box.y*1000000)));
    sqlite3_bind_int(stmt, 5, static_cast<int>(std::round(d.box.width*1000000)));
    sqlite3_bind_int(stmt, 6, static_cast<int>(std::round(d.box.height*1000000)));

    sqlite3_bind_blob(stmt, 7, features.data(), features.size()* sizeof(float), SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
        throw std::logic_error("Cannot store features");

    sqlite3_finalize(stmt);
}

nonstd::optional<FeatureData> FeatureCache::fetch(int frame_no, const Detection &d) const {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT data FROM Features WHERE tag=? AND frame=? AND x=? AND y=? AND w=? AND h=?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "Cannot select data: "<< sqlite3_errmsg(db) << std::endl;
        throw std::logic_error("Cannot select features");
    }
    sqlite3_bind_text(stmt, 1, tag.c_str(), -1, nullptr);
    sqlite3_bind_int(stmt, 2, frame_no);
    sqlite3_bind_int(stmt, 3, static_cast<int>(std::round(d.box.x*1000000)));
    sqlite3_bind_int(stmt, 4, static_cast<int>(std::round(d.box.y*1000000)));
    sqlite3_bind_int(stmt, 5, static_cast<int>(std::round(d.box.width*1000000)));
    sqlite3_bind_int(stmt, 6, static_cast<int>(std::round(d.box.height*1000000)));

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        return nonstd::nullopt;
    }

    if (sqlite3_column_type(stmt, 0) != SQLITE_BLOB) {
        throw std::logic_error("Expected blob result");
    }

    const void* data_void = sqlite3_column_blob(stmt, 0);
    auto* features = static_cast<const float*>(data_void);
    int bytes = sqlite3_column_bytes(stmt, 0);

    int floats = bytes / sizeof(float);
    std::vector<float> f_features;
    for (int i = 0; i < floats; i++) {
        float f = features[i];
        f_features.push_back(f);
    }

    if (sqlite3_step(stmt) != SQLITE_DONE)
        throw std::logic_error("Why is there more than one row inserted?");

    sqlite3_finalize(stmt);

    return FeatureData(f_features);
}



CachedFeatures::CachedFeatures(const FeatureAffinity& features, FeatureCache& cache)
    :features(features), cache(cache)
{}

std::unique_ptr<FeatureData> CachedFeatures::init(const Detection &d, const cv::Mat &frame) const
{
    int frame_no = frame.at<int32_t>(0, 0);
    nonstd::optional<FeatureData> cached = cache.fetch(frame_no, d);
    if (cached) {
        FeatureData data_cached = *cached;
        return std::make_unique<FeatureData>(data_cached);
    } else {
        auto data = features.init(d, frame);
        cache.store(*data, frame_no, d);
        return data;
    }
}

float CachedFeatures::affinity(const FeatureData &detectionData, const FeatureData &trackData) const
{
    return features.affinity(detectionData, trackData);
}

void CachedFeatures::merge(const FeatureData &detectionData, FeatureData &trackData) const
{
    return features.merge(detectionData, trackData);
}
