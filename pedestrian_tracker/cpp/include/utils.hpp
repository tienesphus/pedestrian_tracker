// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "core.hpp"
#include "logging.hpp"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#include <deque>
#include <map>

#include <utils/common.hpp>
#include <opencv2/core.hpp>
///
/// \brief The DetectionLogEntry struct
///
/// An entry describing detected objects on a frame.
///
struct DetectionLogEntry {
    TrackedObjects objects;  ///< Detected objects.
    int frame_idx;           ///< Processed frame index (-1 if N/A).
    double time_ms;          ///< Frame processing time in ms (-1 if N/A).

    ///
    /// \brief DetectionLogEntry default constructor.
    ///
    DetectionLogEntry() : frame_idx(-1), time_ms(-1) {}

    ///
    /// \brief DetectionLogEntry copy constructor.
    /// \param other Detection entry.
    ///
    DetectionLogEntry(const DetectionLogEntry &other)
        : objects(other.objects),
        frame_idx(other.frame_idx),
        time_ms(other.time_ms) {}

    ///
    /// \brief DetectionLogEntry move constructor.
    /// \param other Detection entry.
    ///
    DetectionLogEntry(DetectionLogEntry &&other)
        : objects(std::move(other.objects)),
        frame_idx(other.frame_idx),
        time_ms(other.time_ms) {}

    ///
    /// \brief Assignment operator.
    /// \param other Detection entry.
    /// \return Detection entry.
    ///
    DetectionLogEntry &operator=(const DetectionLogEntry &other) = default;

    ///
    /// \brief Move assignment operator.
    /// \param other Detection entry.
    /// \return Detection entry.
    ///
    DetectionLogEntry &operator=(DetectionLogEntry &&other) {
        if (this != &other) {
            objects = std::move(other.objects);
            frame_idx = other.frame_idx;
            time_ms = other.time_ms;
        }
        return *this;
    }
};

/// Detection log is a vector of detection entries.
using DetectionLog = std::vector<DetectionLogEntry>;

///
/// \brief Save DetectionLog to a txt file in the format
///        compatible with the format of MOTChallenge
///        evaluation tool.
/// \param[in] path -- path to a file to store
/// \param[in] log  -- detection log to store
///
void SaveDetectionLogToTrajFile(const std::string& path,
                                const DetectionLog& log,
                                const std::string& location);

///
/// \brief Print DetectionLog to stdout in the format
///        compatible with the format of MOTChallenge
///        evaluation tool.
/// \param[in] log  -- detection log to print
///
void PrintDetectionLog(const DetectionLog& log, const std::string& location);

///
/// \brief Draws a polyline on a frame.
/// \param[in] polyline Vector of points (polyline).
/// \param[in] color Color (BGR).
/// \param[in,out] image Frame.
/// \param[in] lwd Line width.
///
void DrawPolyline(const std::vector<cv::Point>& polyline,
                  const cv::Scalar& color, cv::Mat* image,
                  int lwd = 5);


///
/// \brief The mouse parameters struct 
///
/// passing frame and mouse points
///
struct MouseParams{
    cv::Mat *frame;
    std::vector<cv::Point2f> mouse_input;
};
///
/// \brief capture coordinates of mouse click, draw circles on those points
/// \param[in] event one of cv::MouseEventTypes constants (EVENT_LBUTTONDOWN,EVENT_RBUTTONDOWN)
/// \param[in] x the x-coordinate of the mouse event.
/// \param[in] y the y-coordinate of the mouse event.
/// \param[in] flags one of the cv::MouseEventFlags constants
/// \param[in] param reference to the MouseParams,contating reference frame and mouse points for drawing circles.
void MouseCallBack(int event, int x, int y, int flags, void* param);

///
/// \brief allow user to configure camera by clicking 7 points
/// \param[in] MouseParams struct containing reference frame and mouse points
void SetCameraPoints(MouseParams* mp);

/// 
/// \brief reading the camera config file (a text file containing points)
/// \param[in] path Path to the file
/// \return points a list of points(x,y coordinates) 
std::vector<cv::Point2f> ReadConfig(const std::string& path);

///
/// \brief writing camera configuration to file (x,y coordinates)
/// \param[in] path Path to the file
/// \return points a list of points(x,y coordinates)
void WriteConfig(const std::string &path, std::vector<cv::Point2f> points);
///
/// \brief Stream output operator for deque of elements.
/// \param[in,out] os Output stream.
/// \param[in] v Vector of elements.
///
template <typename T>
std::ostream &operator<<(std::ostream &os, const std::deque<T> &v) {
    os << "[\n";
    if (!v.empty()) {
        auto itr = v.begin();
        os << *itr;
        for (++itr; itr != v.end(); ++itr) os << ",\n" << *itr;
    }
    os << "\n]";
    return os;
}

///
/// \brief Stream output operator for vector of elements.
/// \param[in,out] os Output stream.
/// \param[in] v Vector of elements.
///
template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
    os << "[\n";
    if (!v.empty()) {
        auto itr = v.begin();
        os << *itr;
        for (++itr; itr != v.end(); ++itr) os << ",\n" << *itr;
    }
    os << "\n]";
    return os;
}

InferenceEngine::Core
LoadInferenceEngine(const std::vector<std::string>& devices,
                    const std::string& custom_cpu_library,
                    const std::string& custom_cldnn_kernels,
                    bool should_use_perf_counter);
