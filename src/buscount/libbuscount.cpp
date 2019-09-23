#include "libbuscount.hpp"
#include "tick_counter.hpp"
#include "cv_utils.hpp"

#include <iostream>
#include <vector>
#include <unistd.h>
#include <deque>

BusCounter::BusCounter(
        Detector& detector,
        Tracker& tracker,
        const WorldConfig &wconf,
        std::function<BusCounter::src_cb_t> src,
        std::function<BusCounter::dest_cb_t> dest,
        std::function<BusCounter::test_exit_t> test_exit,
        std::function<BusCounter::event_handle_t > event_handle
    ) :
        _src(std::move(src)),
        _dest(std::move(dest)),
        _test_exit(std::move(test_exit)),
        _event_handle(std::move(event_handle)),
        _detector(detector),
        _tracker(tracker),
        _world_config(wconf),
        inside_count(0),
        outside_count(0)
{
}

void BusCounter::run(RunStyle style, bool draw)
{
    if(style == RUN_PARALLEL)
        run_parallel(draw);
    else
        run_serial(draw);
}

// abbreviation
template <class T>
using ptr = std::shared_ptr<T>;

void BusCounter::handle_events(const std::vector<Event>& events, const cv::Mat& frame, int frame_no)
{
    for (Event e : events) {
        // handle the events internally
        switch (e) {
            case COUNT_IN:  inside_count++;  break;
            case COUNT_OUT: outside_count++; break;
            case BACK_IN:   outside_count--; break;
            case BACK_OUT:  inside_count--;  break;
            default:
                throw std::logic_error("Unknown event type");
        }

        // handle the event externally
        _event_handle(e, frame, frame_no);
    }
}

//
// A single threaded version of run_parallel. Useful for debugging non-threading related issues.
//
void BusCounter::run_serial(bool do_draw)
{

    TickCounter<> counter;
    while (true) {

        // Read at least once. Skip if source FPS is different from target FPS
        nonstd::optional<std::tuple<cv::Mat, int>> got_frame = _src();
        if (!got_frame)
            break;

        cv::Mat frame = std::get<0>(*got_frame);
        int frame_no = std::get<1>(*got_frame);
        auto detections = _detector.process(frame, frame_no);
        auto events = _tracker.process(detections, frame, frame_no);

        handle_events(events, frame, frame_no);

        if (do_draw) {
            detections.draw(frame);
            _tracker.draw(frame);
            geom::draw_world_count(frame, inside_count, outside_count);
            geom::draw_world_config(frame, _world_config);
        }

        auto fps = counter.process_tick();
        std::cout << "FPS: " << (fps ? *fps : -1) << std::endl;

        _dest(frame);
        if (_test_exit())
            break;
    }
}

void BusCounter::run_parallel(bool do_draw)
{
    std::mutex detection_lock, tracking_lock;

    auto process_frame = [&](cv::Mat frame, int frame_no) -> std::tuple<cv::Mat, int, std::vector<Event>> {
            // Run the detector
            detection_lock.lock();
            auto detections_promise = _detector.start_async(frame, frame_no);
            detection_lock.unlock();

            // Wait for results
            const auto& detections = detections_promise.get();

            // Track the results
            tracking_lock.lock();
            auto events = _tracker.process(detections, frame, frame_no);
            tracking_lock.unlock();

            // Draw stuff if needed
            if (do_draw) {
                detections.draw(frame);
                geom::draw_world_count(frame, inside_count, outside_count);
                geom::draw_world_config(frame, _world_config);
                tracking_lock.lock();
                _tracker.draw(frame);
                tracking_lock.unlock();
            }

            return std::make_tuple(std::move(frame), frame_no, std::move(events));
    };

    const uint8_t MAX_FRAMES = 2;
    std::deque<std::shared_future<std::tuple<cv::Mat, int, std::vector<Event>>>> processing;
    bool finished = false;
    TickCounter<> counter;

    // Initialise a few start-up frames
    for (auto i = 0; i < MAX_FRAMES; i++) {
        // read the frame
        auto got_frame = _src();
        if (!got_frame) {
            break;
        }
        cv::Mat frame = std::get<0>(*got_frame);
        int frame_no = std::get<1>(*got_frame);

        // start the frame processing
        processing.emplace_back(std::async(process_frame, std::move(frame), frame_no));
    }

    // Keep processing frames until we're done
    while (!processing.empty()) {
        std::cout << "WAITING" << std::endl;
        // Retrieve the processed data
        std::tuple<cv::Mat, int, std::vector<Event>> result = processing.front().get();
        processing.pop_front();
        cv::Mat frame = std::get<0>(result);
        int frame_no = std::get<1>(result);
        std::vector<Event> events = std::get<2>(result);

        // Calculate the FPS
        auto fps = counter.process_tick();
        std::cout << "FPS: " << (fps ? *fps : -1) << std::endl;

        std::cout << "DISPLAYING" << std::endl;
        // Output the frame/data
        handle_events(events, frame, frame_no);
        _dest(frame);
        if (_test_exit()) {
            finished = true;
        }

        std::cout << "READING" << std::endl;
        // Get a new frame
        auto got_frame = finished ? nonstd::nullopt : _src();
        if (!got_frame) {
            finished = true;
        } else {
            // start the frame processing
            cv::Mat new_frame = std::get<0>(*got_frame);
            int new_frame_no = std::get<1>(*got_frame);
            processing.emplace_back(std::async(process_frame, std::move(new_frame), new_frame_no));
        }
    }

}