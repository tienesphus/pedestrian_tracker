#ifndef LIBBUSCOUNT_HPP
#define LIBBUSCOUNT_HPP

#include <functional>
#include <optional>

#include <tbb/flow_graph.h>
#include <opencv2/videoio.hpp>

#include "tracker.hpp"
#include "detector.hpp"

typedef cv::Ptr<cv::Mat> PtrMat;

class BusCounter {
public:
    using src_cb_t = std::optional<cv::Mat>();
    using dest_cb_t = void(const cv::Ptr<cv::Mat>);
    using test_exit_t = bool();

    enum RunStyle {
        RUN_PARALLEL,
        RUN_SERIAL
    };

    BusCounter(
            std::unique_ptr<Detector> detector,
            std::unique_ptr<Tracker> tracker,
            WorldConfig &wconf,
            std::function<BusCounter::src_cb_t> src,
            std::function<BusCounter::dest_cb_t> dest,
            std::function<BusCounter::test_exit_t> test_exit
    );

    void run(RunStyle style, double src_fps, bool draw);

private:
    // Pointers to callback functions
    std::function<src_cb_t> _src;
    std::function<dest_cb_t> _dest;
    std::function<test_exit_t> _test_exit;

    // Internal data structures
    std::unique_ptr<Detector> _detector;
    std::unique_ptr<Tracker> _tracker;
    WorldConfig _world_config;

    // How to execute the pipeline.
    void run_parallel(double src_fps, bool draw);
    void run_serial(double src_fps, bool draw);

};

#endif
