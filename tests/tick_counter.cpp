#include "catch.hpp"

#include "tick_counter.hpp"

TEST_CASE( "TickCounter calculates correctly", "[tick_counter]" ) {

    using namespace std::chrono;

    TickCounter<15> counter;

    // call the counter every 50ms
    auto now = high_resolution_clock::now;
    auto start = now();
    for (int calls = 0; calls < 20; ++calls) {
        while (duration_cast<milliseconds>(now() - start).count() < 50 * calls) {
        }
        counter.process_tick();
    }

    float fps = *(counter.getFps());
    REQUIRE(fps > 19.7);
    REQUIRE(fps < 20.3);

}