#ifndef BUS_COUNT_EVENT_HPP
#define BUS_COUNT_EVENT_HPP

#include <string>

enum Event {
    COUNT_IN,  // someone went in
    COUNT_OUT, // someone went out
    BACK_IN,   // someone in then directly back in
    BACK_OUT,  // someone in then directly back out
};

std::string name(Event e);

#endif //BUS_COUNT_EVENT_HPP