#include <utility>

#include <utility>

#include "tracker_reidentify.hpp"

#include <iostream>
#include <utility>

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
    Track(cv::Rect box, float conf, int index, cv::Mat features);

    /**
     * Updates the status of this Track. Updates the world count.
     */
    bool update(const WorldConfig &config, WorldState &world);

    /**
     * Draws the Track onto the given image
     */
    void draw(cv::Mat &img) const;

    cv::Rect box;
    float confidence;
    int index;
    cv::Mat features;

    bool been_inside = false;
    bool been_outside = false;
    bool counted_in = false;
    bool counted_out = false;

    int8_t color; // the hue of the path
    std::vector<cv::Point> path;

};

Track::Track(cv::Rect box, float conf, int index, cv::Mat features):
          box(std::move(box)),
          confidence(conf),
          index(index),
          features(std::move(features)),
          color(rand() % 256)
{}

bool Track::update(const WorldConfig &config, WorldState &world)
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
                world.in_count++;
                counted_in = true;
            }

            if (been_outside && counted_out)
            {
                world.out_count--;
                counted_out = false;
                been_outside = false;
            }
        }      

        if (config.outside.side(p))
        {
            been_outside = true;
            if (been_inside && !counted_out && !counted_in)
            {
                world.out_count++;
                counted_out = true;
            }

            if (been_inside && counted_in)
            {
                world.in_count--;
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


Tracker_RI::Tracker_RI(NetConfig netConfig, WorldConfig world):
        net(cv::dnn::readNetFromModelOptimizer(netConfig.meta, netConfig.model)), netConfig(std::move(netConfig)),
        worldConfig(std::move(world)), state(WorldState(0, 0)), index_count(0)
{
    this->net.setPreferableBackend(this->netConfig.preferred_backend);
    this->net.setPreferableTarget (this->netConfig.preferred_target);
}

Tracker_RI::~Tracker_RI() {
    //for (Track* track : this->tracks) {
    //    delete track;
    //}
}

WorldState Tracker_RI::process(const Detections &detections, const cv::Mat& frame)
{
    merge(detections, frame);
    update();

    // Note: World State is purposely copied out so it is a snapshot
    return state;
}

/**
 * Looks for detections that are near current tracks. If there is something
 * close by, then the detection will be merged into it.
 */
void Tracker_RI::merge(const Detections &detection_results, const cv::Mat& frame)
{

    std::cout << "MERGING" << std::endl;


    struct DetectionExtra {
        const Detection detection;
        cv::Mat features;
        bool merged;
        int index;

        DetectionExtra(Detection d, cv::Mat features, size_t index):
            detection(std::move(d)), features(std::move(features)), merged(false), index(index)
        {}
    };

    struct MergeOption {
        float confidence;
        Track * track;
        DetectionExtra * extra;

        MergeOption(float conf, Track* track, DetectionExtra* detect):
            confidence(conf), track(track), extra(detect)
        {}
    };

    std::cout << "Calc features" << std::endl;
    std::vector<DetectionExtra> detections;
    int index = 0;
    for (Detection detection : detection_results.get_detections()) {

        // Get the track/detection merge confidence
        cv::Mat crop = frame(intersection(detection.box, cv::Rect(0, 0, frame.cols, frame.rows)));
        cv::Mat blob = cv::dnn::blobFromImage(crop, 1, netConfig.size);
        net.setInput(blob);
        cv::Mat result;
        net.forward(result);

        // Add the extra detection info
        detections.emplace_back(detection, result.clone(), index++);
    }


    std::cout << "Calculating differences" << std::endl;

    std::vector<MergeOption> merges;

    for (DetectionExtra &extra : detections) {
        for (std::unique_ptr<Track> &track : this->tracks) {

            float confidence = cosine_similarity(extra.features, track->features);
            std::cout << " d" << extra.index << " x t" << track->index << ": " << confidence << std::endl;
            merges.emplace_back(confidence, track.get(), &extra);

        }
    }

    std::cout << "Sorting differences" << std::endl;
    std::sort(merges.begin(), merges.end(),
            [](const MergeOption &a, const MergeOption &b) -> auto { return a.confidence > b.confidence; }
    );

    std::cout << "Sorted Options: " << std::endl;
    for (const MergeOption &m : merges) {
        std::cout << " d" << m.extra->index << " x t" << m.track->index << ": " << m.confidence << std::endl;
    }

    std::cout << "Finding top merges" << std::endl;

    // iteratively merge the closest of the detection/track combinations
    for (size_t i = 0; i < merges.size(); i++) {
        MergeOption& merge = merges[i];

        // threshold box's that are too far away
        if (merge.confidence < netConfig.thresh) {
            std::cout << " Differences too high" << std::endl;
            break;
        }

        const Detection& d = merge.extra->detection;
        Track* track = merge.track;

        std::cout << "Merging d" << merge.extra->index <<  " and t" << track->index << std::endl;

        // Merge the data
        track->box = d.box;
        track->confidence = std::max(d.confidence, track->confidence);
        track->features = std::move(merge.extra->features);

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

    std::cout << "Adding new tracks" << std::endl;
    // Make new tracks for detections that have not merged
    for (const DetectionExtra &extra : detections) {
        bool dealt_with = extra.merged;
        if (!dealt_with) {
            const Detection &d = extra.detection;
            std::cout << "Making a new box for detection " << d.box << std::endl;

            this->tracks.push_back(std::make_unique<Track>(d.box, d.confidence, index_count++, extra.features));
        }
    }
    
    // Delete tracks with very high overlap
    // This occurs when two detections are produced for the same person
    std::cout << "Deleting similar tracks " << std::endl;
    for (size_t i = 0; i < this->tracks.size(); i++) {
        std::unique_ptr<Track> &track_i = this->tracks[i];
        for (size_t j = i+1; j < this->tracks.size(); j++) {
            std::unique_ptr<Track> &track_j = this->tracks[j];
            
            if (IoU(track_i->box, track_j->box) > 0.95 &&
                    cosine_similarity(track_i->features, track_j->features) > netConfig.thresh) {
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
    }
}

/**
 * Updates the status of each Track. Updates the world count.
 * Deletes old tracks.
 */
void Tracker_RI::update()
{
    this->tracks.erase(std::remove_if(std::begin(this->tracks), std::end(this->tracks),
            [this](std::unique_ptr<Track>& t) {
                return !t->update(this->worldConfig, this->state);
            }
            ), std::end(this->tracks));
}

void Tracker_RI::draw(cv::Mat& img) const
{
    for (const std::unique_ptr<Track>& track : tracks)
        track->draw(img);
}

