#include <utility>

#ifndef BUS_COUNT_READ_SIMULATOR_HPP
#define BUS_COUNT_READ_SIMULATOR_HPP

#include <functional>
#include <cmath>
#include <iostream>

#include <opencv2/videoio.hpp>

#include <optional.hpp>

template <bool keep=false>
class ReadSimulator {

public:

    using src_cb_t = nonstd::optional<cv::Mat>();
    
    explicit ReadSimulator(std::function<src_cb_t> src, float src_fps, const double* time):
            src(std::move(src)),
            src_fps(src_fps),
            time(time),
            current_frame_no(-1),
            time_offset(*time)
    {}

    nonstd::optional<std::tuple<cv::Mat, int>> next() {

        uint32_t frame_no_needed = std::round((*time - time_offset) * src_fps);

        nonstd::optional<std::tuple<cv::Mat, int>> frame;
        do {
            // Must always read at-least one frame
            ++current_frame_no;
            auto img = src();
            if (img) {
                frame = std::make_tuple(*img, current_frame_no);
            } else {
                frame = nonstd::nullopt;
            }
        } while (current_frame_no < frame_no_needed);

        return frame;
    }

    /**
     * Helper function to create a VideoSync from a movie file.
     * @param filename the name of the video
     * @return the syncronised video
     */
    static ReadSimulator<> from_video(const std::string& filename, const double* time) {
        return from_capture(std::make_shared<cv::VideoCapture>(filename), time);
    }

    /**
     * Helper function to create a VideoSync from a opencv Capture.
     * @param cap the video capture to wrap
     * @return the syncronised video
     */
    static ReadSimulator<> from_capture(const std::shared_ptr<cv::VideoCapture>& cap, const double* time) {
        int src_fps = static_cast<int>(cap->get(cv::CAP_PROP_FPS));
        return ReadSimulator<>([cap]() -> auto {
            cv::Mat frame;
            bool result = cap->read(frame);
            return result ? nonstd::optional<cv::Mat>(frame) : nonstd::nullopt;
        }, src_fps, time);
    }


private:
    std::function<src_cb_t> src;
    float src_fps;
    const double* time;
    uint32_t current_frame_no;
    const double time_offset;
};


#endif //BUS_COUNT_READ_SIMULATOR_HPP
