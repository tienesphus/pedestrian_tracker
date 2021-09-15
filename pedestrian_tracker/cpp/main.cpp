// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "core.hpp"
#include "utils.hpp"
#include "tracker.hpp"
#include "descriptor.hpp"
#include "distance.hpp"
#include "detector.hpp"
#include "pedestrian_tracker.hpp"
#include "distance_estimate.hpp"

#include <monitors/presenter.h>
#include <utils/images_capture.h>


#include <chrono>



#include <iostream>
#include <utility>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <gflags/gflags.h>
#include <math.h>
using namespace InferenceEngine;
using ImageWithFrameIndex = std::pair<cv::Mat, int>;

std::unique_ptr<PedestrianTracker>
CreatePedestrianTracker(const std::string& reid_model,
                        const InferenceEngine::Core & ie,
                        const std::string & deviceName,
                        bool should_keep_tracking_info) {
    TrackerParams params;

    if (should_keep_tracking_info) {
        params.drop_forgotten_tracks = false;
        params.max_num_objects_in_track = -1;
    }

    std::unique_ptr<PedestrianTracker> tracker(new PedestrianTracker(params));

    // Load reid-model.
    std::shared_ptr<IImageDescriptor> descriptor_fast =
        std::make_shared<ResizedImageDescriptor>(
            cv::Size(16, 32), cv::InterpolationFlags::INTER_LINEAR);
    std::shared_ptr<IDescriptorDistance> distance_fast =
        std::make_shared<MatchTemplateDistance>();

    tracker->set_descriptor_fast(descriptor_fast);
    tracker->set_distance_fast(distance_fast);

    if (!reid_model.empty()) {
        CnnConfig reid_config(reid_model);
        reid_config.max_batch_size = 16;   // defaulting to 16

        std::shared_ptr<IImageDescriptor> descriptor_strong =
            std::make_shared<DescriptorIE>(reid_config, ie, deviceName);

        if (descriptor_strong == nullptr) {
            throw InferenceEngine::Exception("[SAMPLES] internal error - invalid descriptor"); 
            
        }
        std::shared_ptr<IDescriptorDistance> distance_strong =
            std::make_shared<CosDistance>(descriptor_strong->size());

        tracker->set_descriptor_strong(descriptor_strong);
        tracker->set_distance_strong(distance_strong);
    } else {
        std::cout << "WARNING: Reid model "
            << "was not specified. "
            << "Only fast reidentification approach will be used." << std::endl;
    }

    return tracker;
}

bool ParseAndCheckCommandLine(int argc, char *argv[]) {
    // ---------------------------Parsing and validation of input args--------------------------------------

    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    if (FLAGS_h) {
        showUsage();
        showAvailableDevices();
        return false;
    }

    if (FLAGS_i.empty()) {
        throw std::logic_error("Parameter -i is not set");
    }

    if (FLAGS_m_det.empty()) {
        throw std::logic_error("Parameter -m_det is not set");
    }

    if (FLAGS_m_reid.empty()) {
        throw std::logic_error("Parameter -m_reid is not set");
    }

    if((!FLAGS_out.empty()|| FLAGS_r) && FLAGS_location.empty()){
        throw std::logic_error("Parameter -location is not set for output file");
    }
    return true;
}

int main(int argc, char **argv) {
    try {
        std::cout << "InferenceEngine: " << printable(*GetInferenceEngineVersion()) << std::endl;

        if (!ParseAndCheckCommandLine(argc, argv)) {
            return 0;
        }

        // Reading command line parameters.
        auto det_model = FLAGS_m_det;
        auto reid_model = FLAGS_m_reid;

        auto detlog_out = FLAGS_out;
        auto detlocation = FLAGS_location;

        auto detector_mode = FLAGS_d_det;
        auto reid_mode = FLAGS_d_reid;

        auto custom_cpu_library = FLAGS_l;
        auto path_to_custom_layers = FLAGS_c;
        bool should_use_perf_counter = FLAGS_pc;

        bool should_print_out = FLAGS_r;
       
        bool should_show = !FLAGS_no_show;
        
        int delay = FLAGS_delay;

        auto path_to_config = FLAGS_config;
        bool is_re_config = FLAGS_reconfig;
        auto detlog_out_a = FLAGS_out_a;

        if (!should_show)
            delay = -1;
        should_show = (delay >= 0);

        bool should_save_det_log = !detlog_out.empty();
        bool should_save_det_exlog = !detlog_out_a.empty();
        std::vector<std::string> devices{detector_mode, reid_mode};
        InferenceEngine::Core ie =
            LoadInferenceEngine(
                devices, custom_cpu_library, path_to_custom_layers,
                should_use_perf_counter);

        DetectorConfig detector_confid(det_model);
        ObjectDetector pedestrian_detector(detector_confid, ie, detector_mode);

        bool should_keep_tracking_info = should_save_det_log || should_print_out;
        std::unique_ptr<PedestrianTracker> tracker =
            CreatePedestrianTracker(reid_model, ie, reid_mode,
                                    should_keep_tracking_info);

        std::unique_ptr<ImagesCapture> cap = openImagesCapture(FLAGS_i, FLAGS_loop, FLAGS_first, FLAGS_read_limit);
        double video_fps = cap->fps();
        if (0.0 == video_fps) {
            // the default frame rate for DukeMTMC dataset
            video_fps = 60.0;
        }
        if(is_re_config && path_to_config.empty()){
            throw std::logic_error("Parameter -config is not set(to use -reconfig, -config must be provided)");
        }
        cv::Mat frame = cap->read();
        if (!frame.data) throw std::runtime_error("Can't read an image from the input");
        cv::Size firstFrameSize = frame.size();

        cv::VideoWriter videoWriter;
        if (!FLAGS_o.empty() && !videoWriter.open(FLAGS_o, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                                                  cap->fps(), firstFrameSize)) {
            throw std::runtime_error("Can't open video writer");
        }
        uint32_t framesProcessed = 0;
        cv::Size graphSize{static_cast<int>(frame.cols / 4), 60};
        Presenter presenter(FLAGS_u, 10, graphSize);

        std::cout << "To close the application, press 'CTRL+C' here";
        if (!FLAGS_no_show) {
            std::cout << " or switch to the output window and press ESC key";
        }
        std::cout << std::endl;
        
        std::vector<cv::Point2f> mouse_input,mouse;
        std::vector<cv::Point2f> points;
        MouseParams mp ={&frame,mouse_input};
        cv::Mat roi_frame;
        frame.copyTo(roi_frame);
        DistanceEstimate estimator(frame);
        MouseParams roi ={&roi_frame,mouse};
        DetectionLogExtra extralog;
        std::vector<cv::Point> poly_line;
        if(!path_to_config.empty()){
            
            if(is_re_config){
                cv::namedWindow("Camera-config", 1);
                cv::setMouseCallback("Camera-config", MouseCallBack, (void *) &mp);
                SetPoints(&mp,7,"Camera-config");
                points = mp.mouse_input;
                WriteConfig(FLAGS_config,points);
            }
            else{
                points = ReadConfig(FLAGS_config);
                mp.mouse_input = points;
            }
            DistanceEstimate temp(frame,mp.mouse_input);
            estimator = temp;
        }
        if(should_save_det_exlog){
            cv::namedWindow("ROI-selection", 1);
            cv::setMouseCallback("ROI-selection",MouseCallBack,(void *)&roi);
            SetPoints(&roi,4,"ROI-selection");
        }
        for (unsigned frameIdx = 0; ; ++frameIdx) {

            
            pedestrian_detector.submitFrame(frame, frameIdx);
            pedestrian_detector.waitAndFetchResults();

            TrackedObjects detections = pedestrian_detector.getResults();

            // timestamp in milliseconds
            //uint64_t cur_timestamp = static_cast<uint64_t >(1000.0 / video_fps * frameIdx);
            uint64_t cur_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            tracker->Process(frame, detections, cur_timestamp);

            presenter.drawGraphs(frame);
            // Drawing colored "worms" (tracks).
            frame = tracker->DrawActiveTracks(frame);

            // Drawing all detected objects on a frame by BLUE COLOR
            for (const auto &detection : detections) {
                cv::rectangle(frame, detection.rect, cv::Scalar(255, 0, 0), 3);
            }

            // Drawing tracked detections only by RED color and print ID and detection
            // confidence level.
            for (auto &detection : tracker->TrackedDetections()) {
                cv::rectangle(frame, detection.rect, cv::Scalar(0, 0, 255), 3);
                std::string text = std::to_string(detection.object_id) +
                    " conf: " + std::to_string(detection.confidence);
            
            }
            if(should_save_det_exlog){
                DrawRoi(roi.mouse_input,cv::Scalar(70,70,70),&frame,2);
                for (auto &track : tracker->CheckInRoi(roi.mouse_input)){
                    DetectionLogExtraEntry entry;
                    entry = tracker->GetDetectionLogExtra(track);
                    extralog.emplace(entry.object_id,entry);
                }
            }
            framesProcessed++;

            if (should_show) {
                if(!path_to_config.empty()){
                    estimator.DrawDistance(detections);
                }              
                cv::imshow("dbg", frame);
                char k = cv::waitKey(delay);
                if (k == 27)
                    break;
                presenter.handleKey(k);
               
            }
            if (videoWriter.isOpened() && (FLAGS_limit == 0 || framesProcessed <= FLAGS_limit)) {
                videoWriter.write(frame);
            }
            if (should_save_det_log && (frameIdx % 100 == 0)) {
                DetectionLog log = tracker->GetDetectionLog(true);
                SaveDetectionLogToTrajFile(detlog_out, log, detlocation);
            }
            if (should_save_det_exlog && (frameIdx % 100 == 0)) {

                SaveDetectionLogToTrajFile(detlog_out_a, extralog);
                extralog = DetectionLogExtra();
            }
            frame = cap->read();
            cv::waitKey(20);
            if (!frame.data) break;
            if (frame.size() != firstFrameSize)
                throw std::runtime_error("Can't track objects on images of different size");
        }
        
        if (should_keep_tracking_info) {
            DetectionLog log = tracker->GetDetectionLog(true);
            if (should_save_det_log)
                SaveDetectionLogToTrajFile(detlog_out, log, detlocation);
            if(should_save_det_exlog)
                SaveDetectionLogToTrajFile(detlog_out_a,extralog);
            if (should_print_out)
                PrintDetectionLog(log, detlocation);
        }
        if (should_use_perf_counter) {
            pedestrian_detector.PrintPerformanceCounts(getFullDeviceName(ie, FLAGS_d_det));
            tracker->PrintReidPerformanceCounts(getFullDeviceName(ie, FLAGS_d_reid));
        }

        std::cout << presenter.reportMeans() << '\n';
    }
    catch (const std::exception& error) {
        std::cerr << "[ ERROR ] " << error.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "[ ERROR ] Unknown/internal exception happened." << std::endl;
        return 1;
    }

    std::cout << "Execution successful" << std::endl;

    return 0;
}
