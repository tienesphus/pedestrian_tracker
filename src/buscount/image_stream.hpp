#ifndef BUS_COUNT_IMAGE_STREAM_HPP
#define BUS_COUNT_IMAGE_STREAM_HPP

#include <deque>
#include <opencv2/core/mat.hpp>
#include <mutex>
#include <condition_variable>
#include <thread>

/**
 * Writes a feed of images to disk in a non blocking method. If too many images are fed in,
 * then the images will simply be dropped.
 */
class ImageStreamWriter {
public:

    /**
     * @param filename where to write files to
     * @param the time to sleep between each write (milliseconds)
     */
    ImageStreamWriter(const std::string& filename, int sleep_time);

    ~ImageStreamWriter();

    /**
     * Starts the process that writes the images to the given filename.
     * This method should always be called sometime after contructing the object.
     * The process will stop in the deconstructor
     */
    void start();

    /**
     * Writes an image to the stream. This is non blocking operation.
     * If there are too many images in the queue, the frame is silently dropped
     * @param image the image to write
     */
    void write(const cv::Mat& image);

private:
    std::string filename, temp_filename;
    std::mutex queue_lock;
    std::condition_variable queue_cv;
    std::deque<cv::Mat> queue;
    std::thread output_writer;
    bool running;
    int sleep_time;
};

#endif //BUS_COUNT_IMAGE_STREAM_HPP
