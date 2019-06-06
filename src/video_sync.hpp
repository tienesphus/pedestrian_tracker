#ifndef BUS_COUNT_VIDEO_SYNC_HPP
#define BUS_COUNT_VIDEO_SYNC_HPP

#include <optional>

#include <opencv2/videoio.hpp>

#include "tick_counter.hpp"

class VideoSync {
public:

    explicit VideoSync(const std::string& file);

    std::optional<cv::Mat> read();

    ~VideoSync();

private:
    TickCounter<10> counter;
    int src_fps;
    cv::VideoCapture cap;
};


#endif //BUS_COUNT_VIDEO_SYNC_HPP
