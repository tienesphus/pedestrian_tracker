#ifndef TRACKER_RI_H
#define TRACKER_RI_H

#include "tracker.hpp"

#include <opencv2/dnn.hpp>
#include <inference_engine.hpp>

/** 
 * A jumble of data that follows a detection result around 
 */
class Track;

/**
 * Persistent data needed to know how to track different objects across frames
 */
class Tracker_RI: public Tracker
{
    
public:

    struct NetConfig {
        std::string meta;       // path to the meta file (.xml)
        std::string model;      // path to the model file (.bin)
        cv::Size size;          // Size of input layer
        float thresh;           // confidence to threshold positive detections at (between 0-1)
    };

    /**
     * Constructs a Tracker from the given data
     * @param config the world in/out lines
     * @param threshold the max distance before tracks are considered different
     */
    Tracker_RI(const NetConfig& net, WorldConfig world, InferenceEngine::InferencePlugin &plugin);

    ~Tracker_RI();

    /**
     * Processes some detections
     * @returns a snapshot of the new state of the world
     */
    WorldState process(const Detections &detections, const cv::Mat& frame) override;
    
    /**
     * Draws the current state
     */
    void draw(cv::Mat &img) const override;
    
private:
    // tracker cannot be copied
    Tracker_RI(const Tracker_RI& t);
    Tracker_RI& operator=(const Tracker_RI& t);

    NetConfig netConfig;
    InferenceEngine::ExecutableNetwork network;
    std::string inputName, outputName;

    WorldConfig worldConfig;
    WorldState state;
    std::vector<std::unique_ptr<Track>> tracks;
    int index_count;

    std::vector<float> identify(const cv::Mat &person);
    void merge(const Detections &detections,  const cv::Mat& frame);
    void update();
};

#endif
