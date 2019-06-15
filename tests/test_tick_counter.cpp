#include "catch.hpp"

#include "tick_counter.hpp"

TEST_CASE( "TickCounter calculates correctly", "[tick_counter]" ) {

    using namespace std::chrono;

    TickCounter<15> counter;

    int set_fps = 30;
    auto now = high_resolution_clock::now;
    auto start = now();
    for (int calls = 1; calls < set_fps; ++calls) { // run for a second
        bool one_wait = false;
        while (duration_cast<milliseconds>(now() - start).count() < (calls * 1000/set_fps)) {
            one_wait= true;
        }
        REQUIRE(one_wait); // if this fails, then the computer cant keep up

        nonstd::optional<float> fps = counter.process_tick();

        if (calls > 1) { // allow a little startup time
            REQUIRE(fps.has_value());
            float f_fps = *fps;
            REQUIRE(f_fps > (set_fps-2));
            REQUIRE(f_fps < (set_fps+2));
        }
    }

    float fps = *(counter.getFps());
    REQUIRE(fps > (set_fps-1));
    REQUIRE(fps < (set_fps+1));

}