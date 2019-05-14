#ifndef LIBBUSCOUNT_HPP
#define LIBBUSCOUNT_HPP

#include <tbb/flow_graph.h>
#include <opencv2/videoio.hpp>

#include "world.hpp"
#include "detection.hpp"

class BusCounter {
public:
    using src_cb_t = std::function<bool(cv::Ptr<cv::Mat>)>;
    using dest_cb_t = std::function<std::tuple<cv::Ptr<WorldState>, cv::Ptr<cv::Mat>>(const cv::Ptr<WorldState>)>;

private:
    // Pointers to callback functions
    src_cb_t _src;
    dest_cb_t _dest;

    // Internal data structures
    Detector _detector;
    cv::dnn::Net _net;

    // Functions within the pipeline
    tbb::flow::continue_msg pre_detect(const cv::Ptr<cv::Mat> input);
    cv::Ptr<cv::Mat> detect();
    cv::Ptr<Detections> post_detect(const std::tuple<cv::Ptr<cv::Mat>, cv::Ptr<cv::Mat>>);
    std::tuple<cv::Ptr<WorldState>, cv::Ptr<cv::Mat>> track(std::tuple<cv::Ptr<Detections>, cv::Ptr<WorldState>>);
    tbb::flow::continue_msg monitor();

    void run_parllel(const NetConfigIR &net_config, const WorldConfig world_config);
    void run_series(const NetConfigIR &net_config, const WorldConfig world_config);

public:
    BusCounter(NetConfigIR config, BusCounter::src_cb_t src, BusCounter::dest_cb_t dest);
    void run(const NetConfigIR &net_config, const WorldConfig world_config, bool parallel);
};

#endif
