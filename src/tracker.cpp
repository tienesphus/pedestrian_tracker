#include "tracker.hpp"

Track::Track(const cv::Rect2d &box, float conf, int index):
      box(box),
      confidence(conf),
      index(index)
{
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
    confidence -= 0.05;
    return confidence > 0.2;
}

Tracker::Tracker(WorldConfig config):
    config(config), state(WorldState(cv::Ptr<cv::Mat>(), 0, 0)), index_count(0)
{
}

Tracker::~Tracker()
{
    for (Track* track : tracks)
        delete track;
}

cv::Ptr<WorldState> Tracker::process(Detections &detections) 
{
    for (Detection detection : detections.get_detections()) {
        std::cout << "Looking for merges" << std::endl;
        int replace_index = -1;
        float best_merginess = 0;
        Track* merge_track;

        for (int i = 0; i < tracks.size(); i++)
        {
            Track* track = tracks[i];
            float iou = IoU(track->box, detection.box);
            float merginess = iou;//* track->confidence;
            std::cout << " " + std::to_string(track->index) + ": " + std::to_string(merginess) << std::endl;
            if ((merginess > 0.4) && (merginess > best_merginess))
            {
                best_merginess = merginess;
                replace_index = i;
                merge_track = track;
            }
        }

        // If the track confidence is higher, then just keep using the track's box
        if ((replace_index >= 0) and (merge_track->confidence > detection.confidence))
            continue;
        
        cv::Rect2d box = detection.box;
        if (replace_index >= 0)
        {
            merge_track->box = detection.box;
            merge_track->confidence = detection.confidence;
        } else 
        {
            std::cout << "Making a new box" << std::endl;
            Track* t = new Track(box, detection.confidence, index_count++);
            tracks.push_back(t);
        }
    }
    
    // TODO: return something
    return cv::Ptr<WorldState>();
}


bool Tracker::update(const WorldConfig& config, WorldState& world)
{
    std::vector<Track*> new_tracks;
    for (Track* t : tracks) {
        if (t->update(config, world))
            new_tracks.push_back(t);
        else
            delete t;
    }
    tracks = new_tracks;
}

void Tracker::draw(cv::Mat& img) const
{
    for (Track* track : tracks)
        track->draw(img);
}

