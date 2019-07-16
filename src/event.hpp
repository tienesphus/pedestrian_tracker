#ifndef BUS_COUNT_EVENT_HPP
#define BUS_COUNT_EVENT_HPP

#include <string>

enum Event {
    COUNT_IN,
    COUNT_OUT,
};

std::string name(Event e);

#endif //BUS_COUNT_EVENT_HPP
