#include "tracker_component.hpp"

#include <utility>
#include <iostream>

#include <opencv2/imgproc.hpp>


//  ----------- TRACK ---------------

class Track {
public:
    /**
     * Initialises a Track
     * @param box the bounding box of where the track is
     * @param conf the confidence that the track still exists
     * @param index a unique index for the track
     */
    Track(cv::Rect box, float conf, int index, std::vector<std::unique_ptr<TrackData>> data);

    /**
     * Updates the status of this Track and stores any events in the given list
     */
    bool update(const WorldConfig &config, std::vector<Event>& events);

    /**
     * Draws the Track onto the given image
     */
    void draw(cv::Mat &img) const;

    cv::Rect box;
    float confidence;
    int index;

    bool been_inside = false;
    bool been_outside = false;
    bool counted_in = false;
    bool counted_out = false;

    std::vector<std::unique_ptr<TrackData>> data;

    int8_t color; // the hue of the path
    std::vector<cv::Point> path;

};

Track::Track(cv::Rect box, float conf, int index, std::vector<std::unique_ptr<TrackData>> data):
        box(std::move(box)),
        confidence(conf),
        index(index),
        data(std::move(data)),
        color(rand() % 256)
{}

bool Track::update(const WorldConfig &config, std::vector<Event>& events)
{
    // look only at the boxes center point
    int x = box.x + box.width/2;
    int y = box.y + box.height/4;
    cv::Point p(x, y);

    if (config.in_bounds(p))
    {
        // Count the people based on which direction they pass
        if (config.inside.side(p))
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
                events.push_back(Event::COUNT_IN);
                //world.out_count--;
                counted_out = false;
                been_outside = false;
            }
        }

        if (config.outside.side(p))
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
                events.push_back(Event::COUNT_OUT);
                //world.in_count--;
                counted_in = false;
                been_inside = false;
            }
        }
    }

    path.push_back(p);

    // decrease the confidence (will increase when merging with a detection)
    confidence -= 0.01;
    return confidence > 0.2;
}

void Track::draw(cv::Mat &img) const {
    cv::Scalar clr = hsv2rgb(cv::Scalar(color, 255, confidence*255));
    cv::rectangle(img, box, clr, 1);

    int x = box.x + box.width/2;
    int y = box.y + box.height/4;
    cv::Point p(x, y);
    cv::circle(img, p, 3, clr, 2);

    std::string txt = std::to_string(index);
    cv::Point txt_loc(box.x+5, box.y+15);
    cv::putText(img, txt, txt_loc, cv::FONT_HERSHEY_SIMPLEX, 0.5, clr, 2);

    cv::Point last = path.front();
    for (const cv::Point& pt : path) {
        cv::line(img, pt, last, clr);
        last = pt;
    }
}


//  -----------  TRACKER ---------------

TrackerComp::TrackerComp(const WorldConfig& world):
        worldConfig(world), index_count(0)
{
}

TrackerComp::~TrackerComp() = default;

void TrackerComp::use_affinity(float weighting, std::unique_ptr<Affinity<TrackData>> affinity) {
    this->affinities.emplace_back(std::move(affinity), weighting);
}

std::vector<Event> TrackerComp::process(const Detections &detections, const cv::Mat& frame)
{
    merge(detections, frame);
    return update();
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

std::vector<DetectionExtra> initialise_detections(const std::vector<Detection> &detection_results, const cv::Mat &frame,
                                const std::vector<std::tuple<std::unique_ptr<Affinity<TrackData>>, float>> &affinities)
{
    std::cout << "  Initialise detection info" << std::endl;
    std::vector<DetectionExtra> detections;
    int index = 0;
    for (const Detection& detection : detection_results) {
        std::vector<std::unique_ptr<TrackData>> data;
        for (const std::tuple<std::unique_ptr<Affinity<TrackData>>, float>& tuple : affinities) {
            const std::unique_ptr<Affinity<TrackData>>& affinity = std::get<0>(tuple);
            data.emplace_back(affinity->init(detection, frame));
        }
        // Add the extra detection info
        detections.emplace_back(detection, std::move(data), index++);
    }
    return detections;
}

std::vector<MergeOption> calculate_affinities(std::vector<DetectionExtra>& detections, const std::vector<std::unique_ptr<Track>>& tracks,
        const std::vector<std::tuple<std::unique_ptr<Affinity<TrackData>>, float>>& affinities)
{
    std::cout << "  Calculating affinities" << std::endl;
    std::vector<MergeOption> merges;
    for (DetectionExtra &extra : detections) {
        for (const std::unique_ptr<Track> &track : tracks) {
            float confidence = 0;
            for (size_t i = 0; i < affinities.size(); i++) {
                const auto &tuple = affinities[i];
                const Affinity<TrackData> &affinity = *std::get<0>(tuple);
                float weight = std::get<1>(tuple);

                confidence += weight * affinity.affinity(*extra.data[i], *track->data[i]);
            }
            std::cout << "      d" << extra.index << " x t" << track->index << ": " << confidence << std::endl;
            merges.emplace_back(confidence, track.get(), &extra);
        }
    }
    return merges;
}

void merge_top(std::vector<MergeOption> merges,
         const std::vector<std::tuple<std::unique_ptr<Affinity<TrackData>>, float>>& affinities)
 {

     // TODO tracker merging code is a mess

    // Sort the merges so best merges are first
    std::cout << "    Sorting differences" << std::endl;
    std::sort(merges.begin(), merges.end(),
              [](const MergeOption &a, const MergeOption &b) -> auto { return a.confidence > b.confidence; }
    );
    std::cout << "    Sorted Merges: " << std::endl;
    for (const MergeOption &m : merges) {
        std::cout << "      d" << m.extra->index << " x t" << m.track->index << ": " << m.confidence << std::endl;
    }

    // iteratively merge the closest of the detection/track combinations
    for (size_t i = 0; i < merges.size(); i++) {
        MergeOption& merge = merges[i];

        // threshold box's that are too different
        // TODO remove hardcoded tracker threshold
        if (merge.confidence < 0.3) {
            std::cout << " Differences too high" << std::endl;
            break;
        }

        const Detection& d = merge.extra->detection;
        Track* track = merge.track;

        std::cout << "Merging d" << merge.extra->index <<  " and t" << track->index << std::endl;

        // Merge the data
        track->box = d.box;
        track->confidence = std::max(d.confidence, track->confidence);
        for (size_t j = 0; j < affinities.size(); j++) {
            std::get<0>(affinities[j])->merge(*merge.extra->data[j], *track->data[j]);
        }

        // delete all values in the map with the same track
        // (so we do not merge them twice accidentally)
        merge.extra->merged = true;
        for (size_t j = i+1; j < merges.size(); j++) {
            const MergeOption& merge2 = merges[j];
            if (merge2.track == track || merge2.extra->merged) {
                std::cout << " Ignore option " << j << std::endl;
                merges.erase(merges.begin()+j);
                j--;
            }
        }
    }
}

void add_new_tracks(std::vector<DetectionExtra>& detections, std::vector<std::unique_ptr<Track>>& tracks, int& index_count)
{
    std::cout << "Adding new tracks" << std::endl;
    // Make new tracks for detections that have not merged
    for (DetectionExtra &extra : detections) {
        bool dealt_with = extra.merged;
        if (!dealt_with) {
            const Detection &d = extra.detection;
            std::cout << "Making a new box for detection " << d.box << std::endl;

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
    /*std::cout << "Deleting similar tracks " << std::endl;
    for (size_t i = 0; i < this->tracks.size(); i++) {
        std::unique_ptr<Track> &track_i = this->tracks[i];
        for (size_t j = i+1; j < this->tracks.size(); j++) {
            std::unique_ptr<Track> &track_j = this->tracks[j];

            if (IoU(track_i->box, track_j->box) > 0.95 &&
                cosine_similarity(track_i->data, track_j->data) > netConfig.thresh) {
                if (track_i->confidence > track_j->confidence) {
                    std::cout << "Deleting j: " << j << std::endl;
                    this->tracks.erase(this->tracks.begin()+j);
                    --j;
                } else {
                    std::cout << "Deleting i: " << i << std::endl;
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
void TrackerComp::merge(const Detections &detection_results, const cv::Mat& frame)
{

    // The basic process is:
    // - Initialise extra data on all detections
    // - compute the affinity between all detections and all existing tracks
    // - merge the top detection/track combinations
    // - add new tracks for unmerged detections

    std::cout << "MERGING" << std::endl;

    std::vector<DetectionExtra> detections = initialise_detections(detection_results.get_detections(), frame, this->affinities);
    std::vector<MergeOption> merges = calculate_affinities(detections, this->tracks, this->affinities);
    merge_top(merges, this->affinities);
    add_new_tracks(detections, this->tracks, this->index_count);
    delete_overlapping_tracks(this->tracks, this->affinities);
}

/**
 * Updates the status of each Track. Updates the world count.
 * Deletes old tracks.
 */
std::vector<Event> TrackerComp::update()
{
    std::vector<Event> events;

    this->tracks.erase(std::remove_if(std::begin(this->tracks), std::end(this->tracks),
                                      [this, &events](std::unique_ptr<Track>& t) {
                                          return !t->update(this->worldConfig, events);
                                      }
    ), std::end(this->tracks));

    return events;
}

void TrackerComp::draw(cv::Mat& img) const
{
    for (const std::unique_ptr<Track>& track : tracks)
        track->draw(img);
}

