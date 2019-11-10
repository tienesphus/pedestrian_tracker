#include "image_stream.hpp"

#include <spdlog/spdlog.h>
#include <opencv2/imgcodecs.hpp>

#define STREAM_BUFFER_SIZE (2)

// TODO InputSreamWriter assumes .png filetype.
// I think there will be some really wierd glitches if it isn't png
ImageStreamWriter::ImageStreamWriter(const std::string& filename, int sleep_time)
    :filename(filename), temp_filename(filename + ".tmp.png"), running(false), sleep_time(sleep_time)
{}

ImageStreamWriter::~ImageStreamWriter()
{
    running = false;
    queue_cv.notify_all();
    output_writer.join();
}

void ImageStreamWriter::start() {
    running = true;

    // The output writer does all the actual work
    output_writer = std::thread([this]() {
        while (running) {
            // Read the next image. Wait if none present
            std::unique_lock<std::mutex> lck(queue_lock);
            while (queue.empty()) {
                queue_cv.wait(lck);

                if (!running)
                    return;
            }

            spdlog::debug("Writing frame to .png");
            // The frame must be cloned because it's deconstructor/etc is not thread safe
            const cv::Mat frame = queue.front().clone();
            queue.pop_front();
            lck.unlock();

            // Write the image to a temporary file, then move it across
            cv::imwrite(temp_filename, frame);
            std::rename(temp_filename.c_str(), filename.c_str());

            usleep(1000 * sleep_time);
        }
    });
}

void ImageStreamWriter::write(const cv::Mat& image) {
    // Write to the queue
    std::unique_lock<std::mutex> scope(queue_lock);
    if (queue.size() < STREAM_BUFFER_SIZE) {
        queue.push_back(image);
        spdlog::debug("Writing frame to buffer");
    }

    // notify that the queue has changed
    queue_cv.notify_all();
}