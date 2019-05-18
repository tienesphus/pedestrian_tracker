#ifndef LIBBUSCOUNT_HPP
#define LIBBUSCOUNT_HPP

#include <functional>

#include <tbb/flow_graph.h>
#include <opencv2/videoio.hpp>

#include "world.hpp"
#include "detection.hpp"
#include "tracker.hpp"

class BusCounter {
public:
    using src_cb_t = bool(cv::Ptr<cv::Mat>&);
    using dest_cb_t = void(const cv::Ptr<cv::Mat>);
    using test_exit_t = bool(void);

    enum RunStyle {
        RUN_PARALLEL,
        RUN_SERIAL
    };

private:
    // Pointers to callback functions
    std::function<src_cb_t> _src;
    std::function<dest_cb_t> _dest;
    std::function<test_exit_t> _test_exit;

    // Internal data structures
    Detector _detector;
    WorldConfig _world_config;
    cv::dnn::Net _net;
    Tracker _tracker;

    // Functions within the pipeline
    void pre_detect(const cv::Ptr<cv::Mat>);
    cv::Ptr<cv::Mat> detect(tbb::flow::continue_msg);
    cv::Ptr<Detections> post_detect(const std::tuple<cv::Ptr<cv::Mat>, cv::Ptr<cv::Mat>>);
    std::tuple<cv::Ptr<WorldState>, cv::Ptr<Detections>> track(const cv::Ptr<Detections>);
    cv::Ptr<cv::Mat> draw(std::tuple<cv::Ptr<WorldState>, cv::Ptr<Detections>>);
    cv::Ptr<cv::Mat> no_draw(std::tuple<cv::Ptr<WorldState>, cv::Ptr<Detections>>);

    // How to execute the pipeline.
    void run_parallel(bool draw);
    void run_serial(bool draw);

public:
    BusCounter(
            NetConfigIR &nconf,
            WorldConfig &wconf,
            std::function<BusCounter::src_cb_t> src,
            std::function<BusCounter::dest_cb_t> dest,
            std::function<BusCounter::test_exit_t> test_exit
    );
    void run(RunStyle style, bool draw);
};

#endif
