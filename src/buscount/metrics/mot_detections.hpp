#ifndef BUS_COUNT_MOTDETECTIONS_HPP
#define BUS_COUNT_MOTDETECTIONS_HPP

#include "../detection/detector.hpp"

#include <map>

/**
 * This detector reads directly from the MOT detections file.
 * The file format (at the time of writing this code) was that each line in the detections
 * file should be:
 * <frameno>,<id>,<box_x>,<box_y>,<box_width>,<box_height>,<conf>,<x>,<y>,<z>
 *
 * where:
 *  frameno is the frame number (starting at 1)
 *  id is ignored detections
 *  box_<x,y,width,height> are the box dimensions (in pixels)
 *  conf is the confidence of a detection (between 0-100)
 *  x,y,z are ignored
 */
class DetectorMot : public Detector {
public:
    /**
     * Creates an empty detector
     * @param thresh the threshold to filter detections on (0-1)
     */
    explicit DetectorMot(float thresh, float conf_scale);

    /**
     * Sets where to find the detections file
     * @param detections_file
     */
    void set_file(const std::string& detections_file);

    ~DetectorMot() override;

    Detections process(const cv::Mat &frame, int frame_no) override;

private:
    float thresh;
    float conf_scale;
    std::map<int, Detections> entries;
    std::mutex entry_lock;
};


#endif //BUS_COUNT_MOTDETECTIONS_HPP
