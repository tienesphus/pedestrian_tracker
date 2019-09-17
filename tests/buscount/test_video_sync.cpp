#include <unistd.h>
#include <atomic>

#include <video_sync.hpp>

#include "catch.hpp"

TEST_CASE( "Video Sync skips the correct number of frames", "[video_sync]" ) {

    namespace ch = std::chrono;
    typedef ch::high_resolution_clock Time;
    typedef ch::milliseconds ms;

    float fps_set = 70;
    float fps_call = 30;

    // Create a VideoSync from a continuous set of numbers
    std::atomic<uint32_t> frame_no(0);
    VideoSync<uint32_t, 10> sync([&frame_no]() -> uint32_t {
        return ++frame_no;
    }, fps_set);

    // Request frames from the source at a lower rate than they should be produced
    auto start = Time::now();
    for (int calls = 1; calls < 20; ++calls) {
        // delay by looping
        bool one_wait = false;
        while (ch::duration_cast<ms>(Time::now()-start).count() < (calls *  1000/fps_call)) {
            one_wait = true;
        }
        // ensure that the computer is not too slow
        REQUIRE(one_wait);

        // Get where VideoSync thinks we should be
        std::tuple<uint32_t, int> frame_result = *sync.next();
        uint32_t frame_index = std::get<0>(frame_result);
        uint32_t frame_no_res = std::get<1>(frame_result);

        // frame_no and frame_index should be the same since both start from 0 and count each frame
        REQUIRE(frame_index == frame_no_res);


        // it should actually be every n frames
        float use_every_n_frames = fps_set/fps_call;
        float offset = fmod(frame_index - 1, use_every_n_frames);
        if (offset > use_every_n_frames/2)
            offset -= use_every_n_frames;
        // allow +-1 frame error
        REQUIRE(offset >= -1.01);
        REQUIRE(offset <= 1.01);
    }

}

TEST_CASE( "Video Sync does not segfault with video", "[video_sync]" ) {

    VideoSync<cv::Mat> sync = VideoSync<cv::Mat>::from_video(
            SOURCE_DIR "/../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4");

    auto frame = sync.next();
    REQUIRE(frame.has_value());
    REQUIRE(!std::get<0>(*frame).empty());

    usleep(1000*100);

    frame = sync.next();
    REQUIRE(frame.has_value());
    REQUIRE(!std::get<0>(*frame).empty());

    // Really, this test case is just to make sure it doesn't crash

}
