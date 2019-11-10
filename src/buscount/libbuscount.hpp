#ifndef LIBBUSCOUNT_HPP
#define LIBBUSCOUNT_HPP

#include <functional>

#include <opencv2/videoio.hpp>

#include "tracking/tracker.hpp"
#include "detection/detector.hpp"
#include <optional.hpp>

/**
 * BusCounter handles the main running of the detection/tracking system. It ensures that frames are read in/processed
 * in the correct order and syncronised on correct threads.
 */
class BusCounter {
public:
    using src_cb_t = nonstd::optional<cv::Mat>();
    using dest_cb_t = void(const cv::Mat&);
    using test_exit_t = bool();
    using event_handle_t = void(Event e);

    /**
     * How to run the BusCounter. In theory, parallel should get the exact same results as serial, except slower.
     * In practice, the serial mode helps to debug threading issues
     */
    enum RunStyle {
        RUN_PARALLEL,
        RUN_SERIAL
    };

    BusCounter(
            Detector& detector,
            Tracker& tracker,
            const WorldConfig& wconf,
            std::function<BusCounter::src_cb_t> src,
            std::function<BusCounter::dest_cb_t> dest,
            std::function<BusCounter::test_exit_t> test_exit,
            std::function<BusCounter::event_handle_t> event_handle
    );

    /**
     * Runs the counter in either style. Will block permenantly until the provided source runs out of frames
     * (which is never if a camera is used)
     * @param style parallel or serial
     * @param draw whether drawing should be done or not
     */
    void run(RunStyle style, bool draw);

    /**
     * Updates the current world config. Can be be called from any thread at any time
     * @param config the config to switch to
     */
    void update_world_config(const WorldConfig& config);

private:
    // Pointers to callback functions
    std::function<src_cb_t> _src;
    std::function<dest_cb_t> _dest;
    std::function<test_exit_t> _test_exit;
    std::function<event_handle_t> _event_handle;

    // Internal data structures
    Detector& _detector;
    Tracker& _tracker;
    WorldConfig _world_config;
    std::mutex _config_update;
    WorldConfig* _world_config_waiting;

    int inside_count, outside_count;

    // How to execute the pipeline.
    void run_parallel(bool draw);
    void run_serial(bool draw);
    void handle_events(const std::vector<Event>& events);
};

#endif
