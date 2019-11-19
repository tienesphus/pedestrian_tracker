#ifndef BUS_COUNT_TICK_COUNTER_HPP
#define BUS_COUNT_TICK_COUNTER_HPP

#include <chrono>
#include <list>
#include <atomic>
#include <mutex>

#include <optional.hpp>

/**
 * A basic tick counter for counting the FPS of something.
 * @tparam size the number of elements to store in the moving average calculation
 */
template<size_t size = 10>
class TickCounter
{
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::microseconds us;

public:
    TickCounter(): fps(-1)
    {
    }

    /**
     * Processes a tick
     * @return the new FPS. Note that the first couple frames can't have the FPS calculated, so std::nullopt is returned
     */
    nonstd::optional<float> process_tick()
    {
        // ensure two threads can safely call process_tick at the same time
        // must be locked to ensure the times list is not corrupted
        std::lock_guard<std::mutex> scoped(this->lock);

        // push the current time
        auto current = Time::now();
        times.push_front(current);

        if (times.size() <= 1) {
            // impossible to calculate FPS (yet)
            // Must wait until we have two ticks.
            return nonstd::nullopt;
        }

        // Get the average duration
        auto last = times.back();
        auto duration = current - last;
        us micro = std::chrono::duration_cast<us>(duration);

        // Minus 1 because we measure from the END of the first frame, thus, it's duration
        // does not count.
        long frames = times.size() - 1;
        this->fps = (frames*1000.0f*1000)/micro.count();

        // don't let the list get too big
        if (times.size() > size)
            times.pop_back();

        return this->fps;
    }

    /**
     * Gets the current FPS of this tick counter
     * @return the current FPS
     */
    nonstd::optional<float> getFps() const {
        if (fps < 0)
            return nonstd::nullopt;
        return this->fps;
    }

private:
    std::mutex lock;
    std::list<Time::time_point> times;
    std::atomic<float> fps;
};


#endif //BUS_COUNT_TICK_COUNTER_HPP
