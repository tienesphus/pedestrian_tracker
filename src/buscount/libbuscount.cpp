#include "libbuscount.hpp"
#include "tick_counter.hpp"
#include "cv_utils.hpp"
#include "image_stream.hpp"

#include <spdlog/spdlog.h>

#include <vector>
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
        _world_config_waiting(nullptr),
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

    TickCounter<100> counter;
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
        spdlog::info("FPS: {:f}", (fps ? *fps : -1));

        _dest(frame);
        if (_test_exit())
            break;

        {
            // Update the config to the waiting config
            std::lock_guard<std::mutex> lock(_config_update);
            if (_world_config_waiting != nullptr) {
                _world_config = *_world_config_waiting;
                delete _world_config_waiting;
                _world_config_waiting = nullptr;
            }
        }
    }
}

void BusCounter::run_parallel(bool do_draw)
{
    typedef std::tuple<cv::Mat, int, std::vector<Event>> process_result;

    // these locks ensure that only one thread can access the given resource at a time
    std::mutex detection_lock, tracking_lock, src_lock;

    // these locks ensure that the threads stay in order
    // (otherwise, there may be a case where the first frame has to wait for all latter frames to complete and
    // large jitter will b
    std::mutex src_detect_lock, detect_track_lock;


    ImageStreamWriter writer_live(SOURCE_DIR "/ram_disk/live.png", 100);
    ImageStreamWriter writer_dirty(SOURCE_DIR "/ram_disk/dirty.png", 100);
    writer_live.start();
    writer_dirty.start();

    // define the wor
    auto process_frame = [&]() -> nonstd::optional<process_result> {
        // name this thread something meaningfull
        //pthread_setname_np(pthread_self(), "Detection process");

        // get new frame
        src_lock.lock();
        auto next = _src();

        src_detect_lock.lock();
        src_lock.unlock();

        if (!next) {
            src_detect_lock.unlock();
            return nonstd::nullopt;
        }
        cv::Mat frame = std::get<0>(*next);
        int frame_no = std::get<1>(*next);
        writer_live.write(frame.clone());

        detection_lock.lock();
        src_detect_lock.unlock();

        // Run the detector
        auto detections_promise = _detector.start_async(frame, frame_no);

        detect_track_lock.lock();
        detection_lock.unlock();

        // Wait for results
        const auto& detections = detections_promise.get();

        // Track the results
        tracking_lock.lock();
        detect_track_lock.unlock();

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

    const uint8_t MAX_FRAMES = 5;
    std::deque<std::shared_future<nonstd::optional<process_result>>> processing;
    bool finished = false;
    TickCounter<> counter;

    // Initialise a few start-up frames
    for (auto i = 0; i < MAX_FRAMES; i++) {
        processing.emplace_back(std::async(process_frame));
    }

    // Keep processing frames until we're done
    while (!processing.empty()) {
        spdlog::debug("WAITING");
        // Retrieve the processed data
        nonstd::optional<process_result> result = processing.front().get();
        processing.pop_front();
        if (!result)
            continue; // clear the final few elements from processing queue
        cv::Mat frame = std::get<0>(*result);
        int frame_no = std::get<1>(*result);
        std::vector<Event> events = std::get<2>(*result);

        // Calculate the FPS
        auto fps = counter.process_tick();
        spdlog::info("FPS: {}", fps ? *fps : -1);

        spdlog::debug("DISPLAYING");
        // Output the frame/data
        handle_events(events, frame, frame_no);
        _dest(frame);
        writer_dirty.write(frame.clone());
        if (_test_exit()) {
            finished = true;
        }

        {
            // Update the config to the waiting config
            std::lock_guard<std::mutex> lock(_config_update);
            if (_world_config_waiting != nullptr) {
                _world_config = *_world_config_waiting;
                delete _world_config_waiting;
                _world_config_waiting = nullptr;
            }
        }

        // spin up another process for the sourced frame
        if (!finished)
            processing.emplace_back(std::async(process_frame));
    }

}

void BusCounter::update_world_config(const WorldConfig &config)
{
    std::lock_guard<std::mutex> lock(_config_update);

    // delete the old waiting config (if any)
    if (_world_config_waiting != nullptr) {
        delete _world_config_waiting;
        _world_config_waiting = nullptr;
    }

    // Add the new config
    _world_config_waiting = new WorldConfig(config);
}
