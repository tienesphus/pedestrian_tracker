#include <video_sync.hpp>
#include <iostream>

#include "catch.hpp"

#include "tick_counter.hpp"

TEST_CASE( "Video Sync skips the correct number of frames", "[tick_counter]" ) {

    namespace ch = std::chrono;
    typedef ch::high_resolution_clock Time;
    typedef ch::milliseconds ms;

    float fps_set = 70;
    float fps_call = 30;

    // Create a VideoSync from a continuous set of numbers
    std::atomic_uint32_t frame_no(0);
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
        uint32_t frame_index = *sync.next();

        // it should actually be every n frames
        float use_every_n_frames = fps_set/fps_call;
        float offset = fmod(frame_index - 1, use_every_n_frames);
        if (offset > use_every_n_frames/2)
            offset -= use_every_n_frames;
        // allow +-1 frame error
        REQUIRE(offset >= -1);
        REQUIRE(offset <= 1);
    }

}