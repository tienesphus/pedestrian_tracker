#ifndef TRACKER_COMP_H
#define TRACKER_COMP_H

#include "tracker.hpp"

/**
 * Internal tracking data class
 */
class Track;

/**
 * Internal class used to convert different tracker types.
 */
template <typename T>
class AffAdaptor;

/**
 * Simple base class for AffinityData
 */
class TrackData {
public:
    virtual ~TrackData() = default;
};

/**
 * Pure abstract class which determines the similarity between a new detection and a track.
 *
 * TODO Affinities could be used to do more, such as deleting tracks, drawing all state and counting in/out.
 * It is possible that the Track class can be completely replaced by Affinities
 */
template <typename T>
class Affinity {
public:

    virtual ~Affinity() = default;

    /**
     * Initialises some custom data for a detection. This will always be called for each detection box.
     * The initialised data will be passed latter into the 'affinity' and 'merge' functions.
     * @param d the person detected
     * @param frame the image the person was detected in
     * @return the initialised custom data
     */
    virtual std::unique_ptr<T> init(const Detection& d, const cv::Mat& frame) const = 0;

    /**
     * Determines the affinity between a 'Detection' and a 'Track'.
     * This will be called on EVERY detection/track combination.
     * @param detectionData the data recently created in init()
     * @param trackData data that has been carried over from a previous from through merge()
     * @return the confidence that these two items are alike (between 0 and 1)
     */
    virtual float affinity(const T &detectionData, const T &trackData) const = 0;

    /**
     * Called when it is decided that a detection an a track are the same.
     * @param detectionData the detection data to merge in
     * @param trackData the track data to merge into
     */
    virtual void merge(const T& detectionData, T& trackData) const = 0;

    /**
     * Draws information about this track.
     * TODO Affinity::draw is currently never called
     * @param data the track data
     * @param img the image to draw onto
     */
    virtual void draw(const T& data, cv::Mat &img) const = 0;
};

/**
 * A class that tracks detections moving.
 *
 * This class does not correlate tracks itself, but has "Affinity" components attached to it which do the correlation.
 * This class really just co-ordinates the affinities.
 */
class TrackerComp: public Tracker
{
    
public:

    /**
     * Creates a tracker that is defined by the given world co-ordinates.
     * The created tracker will do nothing by default. Must call "use" to add tracking components
     * @param world the world configuration
     */
    explicit TrackerComp(WorldConfig world);

    ~TrackerComp() override;

    /**
     * Takes ownership of the affinity and uses it to determine likely track/detection merges
     * Multiple affinities can be used. For good results, ensure the weighting of the added components sums to one.
     * @tparam Tracker the Tracking class to add. Must extend Affinity<Data>
     * @tparam Data the type of data the Tracking class requires. Must extend TrackData
     * @param weighting how much emphasis to give this node.
     * @param affinity the tracker to use
     */
    template <typename Tracker, typename Data>
    void use(float weighting, std::unique_ptr<Tracker> affinity) {
        use_affinity(weighting, std::unique_ptr<Affinity<TrackData>>(new AffAdaptor<Data>(std::move(affinity))));
    }

    /**
     * Same as 'use' above, but constructs the unique pointer and affinity from the given arguments.
     */
    template<typename Tracker, typename Data, class... Args>
    void use(float weighting, Args&&... args) {
        use<Tracker, Data>(weighting, std::make_unique<Tracker>(args...));
    }

    /**
     * Processes some detections
     * @returns a snapshot of the new state of the world
     */
    std::vector<Event> process(const Detections &detections, const cv::Mat& frame) override;
    
    /**
     * Draws the current state
     */
    void draw(cv::Mat &img) const override;

private:
    // tracker cannot be copied
    TrackerComp(const TrackerComp& t);
    TrackerComp& operator=(const TrackerComp& t);

    WorldConfig worldConfig;
    std::vector<std::unique_ptr<Track>> tracks;
    int index_count;
    std::vector<std::tuple<std::unique_ptr<Affinity<TrackData>>, float>> affinities;

    void use_affinity(float weighting, std::unique_ptr<Affinity<TrackData>> affinity);
    void merge(const Detections &detections,  const cv::Mat& frame);
    std::vector<Event> update();
};


/**
 * Takes an Affinity component of type Affinity<T> and wraps it into type Affinity<TrackData> so it can be used by
 * TrackerComp. Ideally, this would not be needed, except that c++ does not have covarient templates.
 */
template <typename T>
class AffAdaptor: public Affinity<TrackData>
{
private:
    std::unique_ptr<Affinity<T>> delegate;

public:
    explicit AffAdaptor(std::unique_ptr<Affinity<T>> delegate):
            delegate(std::move(delegate))
    {}

    ~AffAdaptor() override = default;;

    std::unique_ptr<TrackData> init(const Detection& d, const cv::Mat& frame) const override {
        return delegate->init(d, frame);
    }

    float affinity(const TrackData &detectionData, const TrackData &trackData) const override {
        return delegate->affinity(dynamic_cast<const T&>(detectionData), dynamic_cast<const T&>(trackData));
    }

    void merge(const TrackData& detectionData, TrackData& trackData) const override {
        delegate->merge(dynamic_cast<const T&>(detectionData), dynamic_cast<T&>(trackData));
    }

    void draw(const TrackData& data, cv::Mat &img) const override
    {
        delegate->draw(dynamic_cast<const T&>(data), img);
    }

};

#endif // TRACKER_COMP_H
