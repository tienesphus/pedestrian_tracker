#ifndef BUS_COUNT_TICK_COUNTER_HPP
#define BUS_COUNT_TICK_COUNTER_HPP

#include <chrono>
#include <list>
#include <atomic>
#include <mutex>
#include <thread>

/**
 * A basic tick counter for counting the FPS of something.
 * @tparam size the number of elements to store in the moving average calculation
 */
template<size_t size>
class TickCounter
{
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::milliseconds ms;

public:
    TickCounter(): fps(1.0f)
    {
    }

    /**
     * Processes a tick
     * @return the new FPS
     */
    float process_tick()
    {
        // ensure two threads can safely call process_tick at the same time
        std::lock_guard<std::mutex> scoped(this->lock);

        // push the current time
        auto current = Time::now();
        times.push_front(current);

        if (times.size() <= 1) {
            // impossible to calculate FPS (yet)
            // Must wait until we have two ticks.
            return this->fps = 1.0f;
        }

        // Get the average duration
        auto last = times.back();
        auto duration = current - last;
        ms millis = std::chrono::duration_cast<ms>(duration);

        // Minus 1 because we measure from the END of the first frame, thus, it's duration
        // does not count.
        long frames = times.size() - 1;
        this->fps = (float)millis.count()/(frames*1000);

        // don't let the list get too big
        if (times.size() > size)
            times.pop_back();

        return this->fps;
    }

    /**
     * Gets the current FPS of this tick counter
     * @return the current FPS
     */
    float getFps() const {
        return this->fps;
    }

private:
    std::mutex lock;
    std::list<Time::time_point> times;
    std::atomic<float> fps;
};


#endif //BUS_COUNT_TICK_COUNTER_HPP
