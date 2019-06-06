#include "video_sync.hpp"

VideoSync::VideoSync(const std::string& file) {
    cap = cv::VideoCapture(file);
    src_fps = static_cast<int>(cap.get(cv::CAP_PROP_FPS));
}

std::optional<cv::Mat> VideoSync::read() {
    cv::Mat frame;
    bool result;
    std::optional<float> fps;
    do {
        result = cap.read(frame);
        fps = counter.process_tick();
        // keep reading frames until we catch up to src frame rate
    } while (result && fps.has_value() && *fps < src_fps);

    return result ? std::optional(frame) : std::nullopt;
}

VideoSync::~VideoSync() {
    cap.release();
}