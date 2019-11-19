#ifndef BUS_COUNT_DETECTOR_HPP
#define BUS_COUNT_DETECTOR_HPP

#include <future>
#include <opencv2/core/types.hpp>

#include "detection.hpp"

/**
 * A class that handles detection of things. A detector takes in an image and outputs the detections of humans.
 *
 * The detection may be done asyncronously, so implementations must be very careful with thread locking. The detector
 * must guarentee that the output detections are for the correct input detection, but there is no guarentee on what
 * timing order the output is.
 */
class Detector {
public:

    /******** Type definitions ********/

    // TODO: This should be replaced with an autoregistration mechanism for derived types
    enum Type {
        DETECTOR_OPENCV,
        DETECTOR_OPENVINO
    };

    /**
     * Alias for what the 'processing' state of a Detector is
     */
    typedef std::shared_future<Detections> intermediate;


    /******** Object Methods ********/

    Detector() = default;
    virtual ~Detector() = 0;

    /**
     * Begins the process of running inference on an image asyncronously.
     */
    intermediate start_async(const cv::Mat &frame, int frame_no);

    /**
     * Waits for the process to complete
     * @return some unintelligent raw output
     */
    Detections wait_async(const intermediate &request) const;

    /**
     * Runs detection for this frame and outputs the detected things
     * @param frame the RGB input image
     * @return the detections
     */
    virtual Detections process(const cv::Mat &frame, int frame_no) = 0;

private:
    // disallow copying
    Detector(const Detector&);
    Detector& operator=(const Detector&);
};


#endif //BUS_COUNT_DETECTOR_HPP
