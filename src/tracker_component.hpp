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
 * Determines the similarity between a new detection and a track
 */
template <typename T>
class Affinity {
public:

    virtual ~Affinity() = default;

    virtual std::unique_ptr<T> init(const Detection& d, const cv::Mat& frame) const = 0;

    virtual float affinity(const T &detectionData, const T &trackData) const = 0;

    virtual void merge(const T& detectionData, T& trackData) const = 0;

    virtual void draw(const T& data, cv::Mat &img) const = 0;
};

/**
 * A class that tracks detections moving.
 *
 * This class does not correlate tracks itself, but has "Affinity" components attached to it which do the collelation.
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
    WorldState process(const Detections &detections, const cv::Mat& frame) override;
    
    /**
     * Draws the current state
     */
    void draw(cv::Mat &img) const override;

private:
    // tracker cannot be copied
    TrackerComp(const TrackerComp& t);
    TrackerComp& operator=(const TrackerComp& t);

    WorldConfig worldConfig;
    WorldState state;
    std::vector<std::unique_ptr<Track>> tracks;
    int index_count;
    std::vector<std::tuple<std::unique_ptr<Affinity<TrackData>>, float>> affinities;

    void use_affinity(float weighting, std::unique_ptr<Affinity<TrackData>> affinity);
    void merge(const Detections &detections,  const cv::Mat& frame);
    void update();
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
