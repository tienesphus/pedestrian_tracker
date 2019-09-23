#ifndef LIBBUSCOUNT_HPP
#define LIBBUSCOUNT_HPP

#include <functional>

#include <opencv2/videoio.hpp>

#include "tracking/tracker.hpp"
#include "detection/detector.hpp"
#include <optional.hpp>

class BusCounter {
public:
    using src_cb_t = nonstd::optional<std::tuple<cv::Mat, int>>();
    using dest_cb_t = void(const cv::Mat&);
    using test_exit_t = bool();
    using event_handle_t = void(Event e, const cv::Mat& frame, int frame_no);

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

    void run(RunStyle style, bool draw);

private:
    // Pointers to callback functions
    std::function<src_cb_t> _src;
    std::function<dest_cb_t> _dest;
    std::function<test_exit_t> _test_exit;
    std::function<event_handle_t> _event_handle;

    // Internal data structures
    Detector& _detector;
    Tracker& _tracker;
    const WorldConfig& _world_config;

    int inside_count, outside_count;

    // How to execute the pipeline.
    void run_parallel(bool draw);
    void run_serial(bool draw);
    void handle_events(const std::vector<Event>& events, const cv::Mat& frame, int frame_no);
};

#endif
