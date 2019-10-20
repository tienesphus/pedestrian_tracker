#include "cached_features.hpp"
#include "../cv_utils.hpp"

#include <sql_helper.hpp>
#include <spdlog/spdlog.h>

// The DB scale stuff is to remove oddities with floating point numbers
// We scale float numbers from 0-1 to integers from 0-DB_SCALE
const int DB_SCALE = 1000000;

FeatureCache::FeatureCache(const std::string& location, const std::string& tag):
        db(nullptr), tag(tag), feature_lookup({})
{
    if (sqlite3_open(location.c_str(), &db) != SQLITE_OK) {
        throw std::logic_error("Cannot open database");
    }

    // reload the cache
    setTag(tag);
}

FeatureCache::~FeatureCache()
{
    if (db) {
        sqlite3_close(db);
    }
}

cv::Rect2i convert(const cv::Rect2d& b) {
    int x = static_cast<int>(std::round(b.x * DB_SCALE));
    int y = static_cast<int>(std::round(b.y * DB_SCALE));
    int w = static_cast<int>(std::round(b.width * DB_SCALE));
    int h = static_cast<int>(std::round(b.height * DB_SCALE));
    return {x, y, w, h};
}

void FeatureCache::setTag(const std::string& new_tag) {
    this->tag = new_tag;

    lookup_lock.lock();
    feature_lookup.clear();
    lookup_lock.unlock();

    sqlite3_stmt* stmt;
    const char* sql = "SELECT frame, x, y, w, h, data FROM Features WHERE tag=?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot select data: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot select features");
    }
    sqlite3_bind_text(stmt, 1, tag.c_str(), -1, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {

        int frame = sqlite3_column_int(stmt, 0);
        int x = sqlite3_column_int(stmt, 1);
        int y = sqlite3_column_int(stmt, 2);
        int w = sqlite3_column_int(stmt, 3);
        int h = sqlite3_column_int(stmt, 4);


        // read out the blob data
        const void* data_void = sqlite3_column_blob(stmt, 5);
        auto* features = static_cast<const float*>(data_void);
        int bytes = sqlite3_column_bytes(stmt, 5);

        int floats = bytes / sizeof(float);
        std::vector<float> f_features;
        f_features.reserve(floats);
        for (int i = 0; i < floats; i++) {
            float f = features[i];
            f_features.push_back(f);
        }

        lookup_lock.lock();
        feature_lookup[std::make_tuple(frame, x, y, w, h)] = std::move(f_features);
        lookup_lock.unlock();
    }

    sqlite3_finalize(stmt);
}

void FeatureCache::clear() {
    // Delete any existing features in that location
    sqlite3_stmt* delete_stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM Features WHERE tag=?", -1, &delete_stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot delete data: {}", sqlite3_errmsg(db));
        throw std::logic_error("Syntax error in clearing all features from cache");
    }
    sqlite3_bind_text(delete_stmt, 1, tag.c_str(), -1, nullptr);

    if (sqlite3_step(delete_stmt) != SQLITE_DONE) {
        throw std::logic_error("Cannot delete all features");
    }
    sqlite3_finalize(delete_stmt);

    lookup_lock.lock();
    feature_lookup.clear();
    lookup_lock.unlock();
}

void FeatureCache::clear(int frame_no, const Detection& d) {
    cv::Rect2i box = convert(d.box);

    // Delete the detection from the database
    sqlite3_stmt* delete_stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM Features WHERE tag=? AND frame=? AND x=? AND y=? AND w=? AND h=?", -1, &delete_stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot delete data: {}", sqlite3_errmsg(db));
        throw std::logic_error("Syntax error in clear single feature");
    }
    sqlite3_bind_text(delete_stmt, 1, tag.c_str(), -1, nullptr);
    sqlite3_bind_int(delete_stmt, 2, frame_no);
    sqlite3_bind_int(delete_stmt, 3, box.x);
    sqlite3_bind_int(delete_stmt, 4, box.y);
    sqlite3_bind_int(delete_stmt, 5, box.width);
    sqlite3_bind_int(delete_stmt, 6, box.height);

    if (sqlite3_step(delete_stmt) != SQLITE_DONE) {
        throw std::logic_error("Cannot delete feature");
    }

    sqlite3_finalize(delete_stmt);

    // Delete the detection from the ram database
    lookup_lock.lock();
    feature_lookup.erase(std::make_tuple(frame_no, box.x, box.y, box.width, box.height));
    lookup_lock.unlock();
}

void FeatureCache::store(const FeatureData &data, int frame_no, const Detection &d) {

    // Delete any existing features in that location
    clear(frame_no, d);

    // Add new features
    std::vector<float> features = data.features;

    cv::Rect2i box = convert(d.box);

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO Features (tag, frame, x, y, w, h, data) VALUES (?, ?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Cannot insert data: {}", sqlite3_errmsg(db));
        throw std::logic_error("Cannot insert features");
    }

    if (sqlite3_bind_text(stmt, 1, tag.c_str(), -1, nullptr) != SQLITE_OK) {
        throw std::logic_error("Cannot bind data");
    }
    sqlite3_bind_int(stmt, 2, frame_no);

    sqlite3_bind_int(stmt, 3, box.x);
    sqlite3_bind_int(stmt, 4, box.y);
    sqlite3_bind_int(stmt, 5, box.width);
    sqlite3_bind_int(stmt, 6, box.height);

    sqlite3_bind_blob(stmt, 7, features.data(), features.size()* sizeof(float), SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
        throw std::logic_error("Cannot store features");

    sqlite3_finalize(stmt);

    // store it locally too
    lookup_lock.lock();
    feature_lookup[std::make_tuple(frame_no, box.x, box.y, box.width, box.height)] = features;
    lookup_lock.unlock();
}

nonstd::optional<FeatureData> FeatureCache::fetch(int frame_no, const Detection &d) const {

    cv::Rect2i box = convert(d.box);

    std::lock_guard<std::mutex> lock(const_cast<FeatureCache*>(this)->lookup_lock);
    auto index = feature_lookup.find(std::make_tuple(frame_no, box.x, box.y, box.width, box.height));
    if (index == feature_lookup.end()) {
        return nonstd::nullopt;
    } else {
        return FeatureData(index->second);
    }
}



CachedFeatures::CachedFeatures(const FeatureAffinity& features, FeatureCache& cache)
    :features(&features), cache(cache)
{}

CachedFeatures::CachedFeatures(FeatureCache& cache)
        :features(nullptr), cache(cache)
{}

std::unique_ptr<FeatureData> CachedFeatures::init(const Detection &d, const cv::Mat &frame, int frame_no) const
{
    nonstd::optional<FeatureData> cached = cache.fetch(frame_no, d);
    if (cached) {
        FeatureData data_cached = *cached;
        return std::make_unique<FeatureData>(data_cached);
    } else {
        if (features) {
            auto data = features->init(d, frame, frame_no);
            cache.store(*data, frame_no, d);
            return data;
        } else {
            throw std::logic_error("Uknown detection");
        }
    }
}

float CachedFeatures::affinity(const FeatureData &detectionData, const FeatureData &trackData) const
{
    return geom::cosine_similarity(detectionData.features, trackData.features);
}

void CachedFeatures::merge(const FeatureData &detectionData, FeatureData &trackData) const
{
    trackData.features = detectionData.features;
}
