#ifndef BUS_COUNT_VIDEO_SYNC_HPP
#define BUS_COUNT_VIDEO_SYNC_HPP

#include <functional>
#include <cmath>
#include <iostream>

#include <opencv2/videoio.hpp>

#include <optional.hpp>

/**
 * Takes an input source and ensures that it runs in real time. Does so by measuring the time between calls and
 * requesting the input to serve multiple frames if it is behind schedule. It is assumed that the requests are at a
 * lower frequency than the source.
 * @tparam T the output type
 * @tparam averaging the number of frames to average timing over
 */
template <class T, size_t averaging = 10>
class VideoSync {
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::milliseconds ms;

public:

    using src_cb_t = nonstd::optional<T>();

    /**
     * Constructs a VideoSync from the given lambda function
     * @param src a function which provides frames
     * @param src_fps the rate to provide frames at
     */
    VideoSync(std::function<src_cb_t> src, float src_fps):
            src_fps(src_fps),
            src(src),
            current_frame_no(0)
    {}

    /**
     * Grabs the next frame from the stream. Drops frames if we are behind
     */
    nonstd::optional<T> next() {

        // Calculate how many frames we need to read to catch up
        int catchup = 1;
        if (src_time.size() > 0) {
            // only on the second call can we think about how many frames are actually needed
            auto first = src_time.front();
            auto duration = Time::now() - std::get<0>(first);
            auto millis = std::chrono::duration_cast<ms>(duration).count();
            auto processed_frames = current_frame_no - std::get<1>(first);
            float must_read = src_fps * millis / 1000 - processed_frames;
            catchup = std::max(static_cast<int>(std::round(must_read)), 1);
        }

        // read a bunch of frames out
        nonstd::optional<T> frame;
        do {
            // current_frame_no will eventually overflow, but that's okay because we only ever
            // take a difference of values
            ++current_frame_no;
            frame = src();
        } while (frame && --catchup > 0);

        // record our new state
        src_time.emplace_back(Time::now(), current_frame_no);

        // Don't let the history get too large
        if (src_time.size() > averaging) {
            src_time.erase(src_time.begin(), src_time.begin()+1);
        }

        return frame;
    }

    /**
     * Helper function to create a VideoSync from a movie file.
     * @param filename the name of the video
     * @return the syncronised video
     */
    static VideoSync<cv::Mat, averaging> from_video(const std::string& filename) {
        return from_capture(std::make_shared<cv::VideoCapture>(filename));
    }

    /**
     * Helper function to create a VideoSync from a opencv Capture.
     * @param cap the video capture to wrap
     * @return the syncronised video
     */
    static VideoSync<cv::Mat, averaging> from_capture(const std::shared_ptr<cv::VideoCapture> cap) {
        int src_fps = static_cast<int>(cap->get(cv::CAP_PROP_FPS));
        return VideoSync([cap]() -> auto {
            cv::Mat frame;
            bool result = cap->read(frame);
            return result ? nonstd::optional<T>(frame) : nonstd::nullopt;
        }, src_fps);
    }

private:

    float src_fps;
    std::function<src_cb_t> src;

    std::vector<std::tuple<Time::time_point, size_t>> src_time;
    uint16_t current_frame_no;
};

#endif //BUS_COUNT_VIDEO_SYNC_HPP
