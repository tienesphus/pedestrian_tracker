// Project includes
#include "tracker_component.hpp"
#include "../cv_utils.hpp"

// C++ includes
#include <deque>
#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>

//  ----------- TRACK ---------------

class Track {
public:
    /**
     * Initialises a Track
     * @param box the bounding box of where the track is
     * @param conf the confidence that the track still exists
     * @param index a unique index for the track
     */
    Track(cv::Rect2f box, float conf, int index, std::vector<std::unique_ptr<TrackData>> data);

    /**
     * Updates the status of this Track and stores any events in the given list
     */
    bool update(const WorldConfig &config, std::vector<Event>& events, double conf_decrease_rate, double conf_thresh, int deltaFrames);

    /**
     * Draws the Track onto the given image
     */
    void draw(cv::Mat &img) const;

    cv::Rect2f box;
    float confidence;
    int index;

    bool been_inside = false;
    bool been_outside = false;
    bool counted_in = false;
    bool counted_out = false;

    std::vector<std::unique_ptr<TrackData>> data;

    int8_t color; // the hue of the path
    std::deque<geom::Point> path;
    bool wasDetected;

};

Track::Track(cv::Rect2f box, float conf, int index, std::vector<std::unique_ptr<TrackData>> data):
        box(std::move(box)),
        confidence(conf),
        index(index),
        data(std::move(data)),
        color(rand() % 256),
        wasDetected(true)
{}

bool Track::update(const WorldConfig &config, std::vector<Event>& events, double conf_decrease_rate, double conf_thresh, int deltaFrames)
{
    // look only at the boxes center point
    float x = box.x + box.width/2;
    float y = box.y + box.height/4;
    geom::Point p(x, y);

    if (config.in_bounds(p))
    {
        // Count the people based on which direction they pass
        if (config.inside(p))
        {
            been_inside = true;
            if (been_outside && !counted_in && !counted_out)
            {
                events.push_back(Event::COUNT_IN);
                //world.in_count++;
                counted_in = true;
            }

            if (been_outside && counted_out)
            {
                events.push_back(Event::BACK_IN);
                //world.out_count--;
                counted_out = false;
                been_outside = false;
            }
        }

        if (config.outside(p))
        {
            been_outside = true;
            if (been_inside && !counted_out && !counted_in)
            {
                events.push_back(Event::COUNT_OUT);
                //world.out_count++;
                counted_out = true;
            }

            if (been_inside && counted_in)
            {
                events.push_back(Event::BACK_OUT);
                //world.in_count--;
                counted_in = false;
                been_inside = false;
            }
        }
    }

    path.push_back(p);
    if (path.size() > 50)
        path.pop_front();

    // decrease the confidence (will increase when merging with a detection)
    confidence -= conf_decrease_rate * deltaFrames;
    return confidence > conf_thresh;
}

void Track::draw(cv::Mat &img) const {
    float w = img.cols;
    float h = img.rows;
    cv::Scalar clr = geom::hsv2rgb(cv::Scalar(color, 255, confidence*255));

    cv::rectangle(img, cv::Rect2f(box.x*w, box.y*h, box.width*w, box.height*h), clr, 1);

    float x = box.x + box.width/2;
    float y = box.y + box.height/4;
    cv::Point2f p(x*w, y*h);
    cv::circle(img, p, 3, clr, 2);

    std::string txt = std::to_string(index);
    cv::Point2f txt_loc((box.x*w)+5, (box.y*h)+15);
    cv::putText(img, txt, txt_loc, cv::FONT_HERSHEY_SIMPLEX, 0.5, clr, 2);

    geom::Point last = path.front();
    for (const geom::Point& pt : path) {
        cv::line(img, cv::Point2f(pt.x*w, pt.y*h), cv::Point2f(last.x*w, last.y*h), clr);
        last = pt;
    }
}


//  -----------  TRACKER ---------------

TrackerComp::TrackerComp(float merge_thresh, double conf_decrease_rate, double conf_thresh,
                         std::function<void(const cv::Mat&, uint32_t, int, const cv::Rect2f&, float conf)> track_listener):
        merge_thresh(merge_thresh), index_count(0),
        conf_decrease_rate(conf_decrease_rate), conf_thresh(conf_thresh), pre_frame_no(-1), track_listener(std::move(track_listener))
{
}

TrackerComp::~TrackerComp() = default;

void TrackerComp::use_affinity(float weighting, std::unique_ptr<Affinity<TrackData>> affinity) {
    this->affinities.emplace_back(std::move(affinity), weighting);
}

std::vector<Event> TrackerComp::process(const WorldConfig& config, const Detections &detections, const cv::Mat& frame, int frame_no)
{
    // Reset all tracks detection status
    for (const auto& track : tracks) {
        track->wasDetected = false;
    }

    // merge detections into the tracks
    merge(detections, frame, frame_no);

    // notify track listener of all changes
    if (track_listener != nullptr) {
        for (const auto& track : tracks) {
            if (track->wasDetected) // don't notify about dieing tracks
                track_listener(frame, frame_no, track->index, track->box, track->confidence);
        }
    }

    // calculate the number of frames the system has moved by
    int delta = frame_no - pre_frame_no;
    if (delta <= 0)
        delta = 1;
    pre_frame_no = frame_no;

    return update(delta, config);
}


struct DetectionExtra {
    const Detection detection;
    std::vector<std::unique_ptr<TrackData>> data;
    bool merged;
    int index;

    DetectionExtra(Detection d, std::vector<std::unique_ptr<TrackData>> data, size_t index):
            detection(std::move(d)), data(std::move(data)), merged(false), index(index)
    {}

    DetectionExtra(DetectionExtra&& other) noexcept :
            detection(other.detection),
            data(std::move(other.data)),
            merged(other.merged),
            index(other.index)
    {
    }
};

struct MergeOption {
    float confidence;
    Track * track;
    DetectionExtra * extra;

    MergeOption(float conf, Track* track, DetectionExtra* detect):
            confidence(conf), track(track), extra(detect)
    {}
};

std::vector<DetectionExtra> initialise_detections(const std::vector<Detection> &detection_results, const cv::Mat &frame, int frame_no,
                                const std::vector<std::tuple<std::unique_ptr<Affinity<TrackData>>, float>> &affinities)
{
    spdlog::debug("  Initialise detection info");
    std::vector<DetectionExtra> detections;
    int index = 0;
    for (const Detection& detection : detection_results) {
        std::vector<std::unique_ptr<TrackData>> data;
        for (const std::tuple<std::unique_ptr<Affinity<TrackData>>, float>& tuple : affinities) {
            const std::unique_ptr<Affinity<TrackData>>& affinity = std::get<0>(tuple);
            data.emplace_back(affinity->init(detection, frame, frame_no));
        }
        // Add the extra detection info
        detections.emplace_back(detection, std::move(data), index++);
    }
    return detections;
}

std::vector<MergeOption> calculate_affinities(std::vector<DetectionExtra>& detections, const std::vector<std::unique_ptr<Track>>& tracks,
        const std::vector<std::tuple<std::unique_ptr<Affinity<TrackData>>, float>>& affinities)
{
    spdlog::debug("  Calculating affinities");
    std::vector<MergeOption> merges;
    for (DetectionExtra &extra : detections) {
        for (const std::unique_ptr<Track> &track : tracks) {
            float confidence = 1.0f;
            for (size_t i = 0; i < affinities.size(); i++) {
                const auto &tuple = affinities[i];
                const Affinity<TrackData> &affinity = *std::get<0>(tuple);
                float weight = std::get<1>(tuple);

                // confidence is multiplication of all others.
                // max(0,..) is needed so negative times a negative does not become big
                confidence *= weight * std::max(0.0f, affinity.affinity(*extra.data[i], *track->data[i]));
            }
            spdlog::trace("      d{} x t{}: {}", extra.index, track->index, confidence);
            merges.emplace_back(confidence, track.get(), &extra);
        }
    }
    return merges;
}

void merge_top(std::vector<MergeOption> merges,
         const std::vector<std::tuple<std::unique_ptr<Affinity<TrackData>>, float>>& affinities, float merge_thresh)
 {

     // TODO tracker merging code is a mess

    // Sort the merges so best merges are first
    spdlog::debug( "    Sorting differences");
    std::sort(merges.begin(), merges.end(),
              [](const MergeOption &a, const MergeOption &b) -> auto { return a.confidence > b.confidence; }
    );
     spdlog::debug("    Sorted Merges: ");
    for (const MergeOption &m : merges) {
        spdlog::trace("      d{} x t{}: {}", m.extra->index, m.track->index, m.confidence);
    }

    // iteratively merge the closest of the detection/track combinations
    for (size_t i = 0; i < merges.size(); i++) {
        MergeOption& merge = merges[i];

        // threshold box's that are too different
        if (merge.confidence < merge_thresh) {
            spdlog::trace(" Differences too high ({})", i);
            break;
        }

        const Detection& d = merge.extra->detection;
        Track* track = merge.track;

        // Merge the data
        spdlog::trace("Merging d{} and t{}", merge.extra->index, track->index);
        track->box = d.box;
        track->confidence = std::max(d.confidence, track->confidence);
        track->wasDetected = true;
        for (size_t j = 0; j < affinities.size(); j++) {
            std::get<0>(affinities[j])->merge(*merge.extra->data[j], *track->data[j]);
        }

        // delete all values in the map with the same track
        // (so we do not merge them twice accidentally)
        merge.extra->merged = true;
        for (size_t j = i+1; j < merges.size(); j++) {
            const MergeOption& merge2 = merges[j];
            if (merge2.track == track || merge2.extra->merged) {
                spdlog::trace(" Ignore option {}", j);
                merges.erase(merges.begin()+j);
                j--;
            }
        }
    }
}

void add_new_tracks(std::vector<DetectionExtra>& detections, std::vector<std::unique_ptr<Track>>& tracks, int& index_count)
{
    spdlog::debug("Adding new tracks");
    // Make new tracks for detections that have not merged
    for (DetectionExtra &extra : detections) {
        bool dealt_with = extra.merged;
        if (!dealt_with) {
            const Detection &d = extra.detection;
            spdlog::trace("Making a new box for detection ({},{}) ({}x{})", d.box.x, d.box.y, d.box.width, d.box.height);

            tracks.push_back(std::make_unique<Track>(d.box, d.confidence, index_count++, std::move(extra.data)));
        }
    }
}

void delete_overlapping_tracks(std::vector<std::unique_ptr<Track>>&,
        const std::vector<std::tuple<std::unique_ptr<Affinity<TrackData>>, float>>&)
{
    // Delete tracks with very high overlap
    // This occurs when two detections are produced for the same person
    // TODO how to delete overlapping tracks
    /*
    spdlog::debug("Deleting similar tracks ");
    for (size_t i = 0; i < this->tracks.size(); i++) {
        std::unique_ptr<Track> &track_i = this->tracks[i];
        for (size_t j = i+1; j < this->tracks.size(); j++) {
            std::unique_ptr<Track> &track_j = this->tracks[j];

            if (IoU(track_i->box, track_j->box) > 0.95 &&
                cosine_similarity(track_i->data, track_j->data) > netConfig.thresh) {
                if (track_i->confidence > track_j->confidence) {
                    spdlog::debug("Deleting j: {}", j);
                    this->tracks.erase(this->tracks.begin()+j);
                    --j;
                } else {
                    spdlog::debug("Deleting i: {}", i);
                    this->tracks.erase(this->tracks.begin()+i);
                    --i;
                    break;
                }
            }
        }
    }*/
}

/**
 * Looks for detections that are near current tracks. If there is something
 * close by, then the detection will be merged into it.
 */
void TrackerComp::merge(const Detections &detection_results, const cv::Mat& frame, int frame_no)
{

    // The basic process is:
    // - Initialise extra data on all detections
    // - compute the affinity between all detections and all existing tracks
    // - merge the top detection/track combinations
    // - add new tracks for unmerged detections

    spdlog::debug("MERGING");

    std::vector<DetectionExtra> detections = initialise_detections(detection_results.get_detections(), frame, frame_no, this->affinities);
    std::vector<MergeOption> merges = calculate_affinities(detections, this->tracks, this->affinities);
    merge_top(merges, this->affinities, this->merge_thresh);
    add_new_tracks(detections, this->tracks, this->index_count);
    delete_overlapping_tracks(this->tracks, this->affinities);
}

/**
 * Updates the status of each Track. Updates the world count.
 * Deletes old tracks.
 */
std::vector<Event> TrackerComp::update(int deltaFrames, const WorldConfig& config)
{
    std::vector<Event> events;

    this->tracks.erase(std::remove_if(std::begin(this->tracks), std::end(this->tracks),
                                      [this, &events, deltaFrames, &config](std::unique_ptr<Track>& t) {
                                          return !t->update(config, events, this->conf_decrease_rate, this->conf_thresh, deltaFrames);
                                      }
    ), std::end(this->tracks));

    return events;
}

void TrackerComp::draw(cv::Mat& img) const
{
    for (const std::unique_ptr<Track>& track : tracks)
        track->draw(img);
}

