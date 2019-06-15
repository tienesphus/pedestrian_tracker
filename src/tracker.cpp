#include "tracker.hpp"

#include <iostream>
#include <utility>

#include <opencv2/imgproc.hpp>



//  ----------- TRACK ---------------

class Track
{
    friend class Tracker;

    private:
    /**
     * Initialises a Track
     * @param box the bounding box of where the track is
     * @param conf the confidence that the track still exists
     * @param index a unique index for the track
     */
    Track(cv::Rect box, float conf, int index);

    /**
     * Updates the status of this Track. Updates the world count.
     */
    bool update(const WorldConfig& config, WorldState& world);

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
    };

    Track::Track(cv::Rect box, float conf, int index):
          box(std::move(box)),
          confidence(conf),
          index(index)
    {
}

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

    // decrease the confidence (will increase when merging with a detection)
    confidence -= 0.1;
    return confidence > 0.2;
}

void Track::draw(cv::Mat &img) const {
    cv::Scalar clr(0, confidence*255, 0);
    cv::rectangle(img, box, clr, 1);
    
    int x = box.x + box.width/2;
    int y = box.y + box.height/4;
    cv::Point p(x, y);
    cv::circle(img, p, 3, clr, 2);
    
    std::string txt = std::to_string(index);
    cv::Point txt_loc(box.x+5, box.y+15);
    cv::putText(img, txt, txt_loc, cv::FONT_HERSHEY_SIMPLEX, 0.5, clr, 2);
}


//  -----------  TRACKER ---------------

Tracker::Tracker(WorldConfig config):
    config(std::move(config)), state(WorldState(0, 0)), index_count(0)
{
}

Tracker::~Tracker()
{
    for (Track* track : tracks)
        delete track;
}

WorldState Tracker::process(const Detections &detections, const cv::Mat&)
{
    // Note: 'frame' is currently unused, but included as it will be used in the future
    // Note 2: World State is purposely copied out so it is a snapshot

    merge(detections);
    update();
    
    return state;
}

// Gets the square of the distance between two boxes.
int _difference(const cv::Point& center_a, const cv::Point& center_b) {
    int dx = center_a.x - center_b.x;
    int dy = center_a.y - center_b.y;
    
    return dx*dx + dy*dy;
}

/**
 * Looks for detections that are near current tracks. If there is something
 * close by, then the detection will be merged into it.
 */
void Tracker::merge(const Detections &detection_results)
{
    std::cout << "MERGING" << std::endl;
    std::vector<Detection> detections = detection_results.get_detections();
    
    std::vector<std::tuple<int, int, Track*>> confidences; // vector<tuple<difference, detect_index, track>>
    std::vector<bool> detection_delt_with;
    
    std::cout << "Calculating differences" << std::endl;
    // build a map of differences between all tracks and all detections 
    for (size_t i = 0; i < detections.size(); i++) {
        Detection detection = detections[i];

        // determine the similarity between this track and the others
        for (Track* track : this->tracks) {
            int difference = _difference(
                cv::Point(   track->box.x +    track->box.width/2,     track->box.y +    track->box.height/4),
                cv::Point(detection.box.x + detection.box.width/2,  detection.box.y + detection.box.height/4)
            );
            std::cout << " d" << i << " x t" << track->index << ": " << difference << std::endl;
            confidences.emplace_back(difference, i, track);
        }
      
        // initialise 'delt_with' to false
        detection_delt_with.push_back(false);
    }  
	
    std::cout << "Sorting differences" << std::endl;
    // order confidences so that lowest difference is first
    // (sort works by default of the first element of the tuple)
    std::sort(confidences.begin(), confidences.end());

    std::cout << "Finding merges" << std::endl;
    // iteratively merge the closest of the detection/track combinations
    for (size_t i = 0; i < confidences.size(); i++) {
        std::tuple<int, int, Track*> data = confidences[i];
        int difference = std::get<0>(data);
        int detection_index = std::get<1>(data);
        Track* track = std::get<2>(data);

        // threshold box's that are too far away (60px)
        if (difference > 60*60) {
            std::cout << " Differences too high" << std::endl;
            break;
        }
      
        std::cout << "Merging d" << detection_index <<  " and t" << track->index << std::endl;
        Detection d = detections[detection_index];
        track->box = d.box;
        track->confidence = std::max(d.confidence, track->confidence);

        // delete all values in the map with the same detection/track
        // (so we do not merge them twice accidentally)
        for (size_t j = i+1; j < confidences.size(); j++) {
            int detection_index_2 = std::get<1>(confidences[j]);
            Track* track_2 = std::get<2>(confidences[j]);
            
            if ((track_2 == track) || (detection_index_2 == detection_index)) {
                confidences.erase(confidences.begin()+j);
            }
        }
      
        // This detection has been merged, so flag that is shouldn't be
        // added as a new track
        detection_delt_with[detection_index] = true;  
    }

    std::cout << "Adding new tracks" << std::endl;
    // Make new tracks for detections that have not merged
    for (size_t i = 0; i < detection_delt_with.size(); i++) {
        bool delt_with = detection_delt_with[i];
        if (!delt_with) {
            Detection d = detections[i];
            std::cout << "Making a new box for detection " << i << std::endl;
            this->tracks.push_back(new Track(d.box, d.confidence, index_count++));
        }
    }
}

/**
 * Updates the status of each Track. Updates the world count.
 * Deletes old tracks.
 */
void Tracker::update()
{
    std::vector<Track*> new_tracks;
    for (Track* t : this->tracks) 
    {
        if (t->update(this->config, this->state))
            new_tracks.push_back(t);
        else
            delete t;
    }
    this->tracks = new_tracks;
}

void Tracker::draw(cv::Mat& img) const
{
    for (Track* track : tracks)
        track->draw(img);
}

