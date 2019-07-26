#include <stdexcept>
#include "event.hpp"

std::string name(Event e)
{
    switch (e)
    {
        case COUNT_IN: return "COUNT_IN";
        case COUNT_OUT: return "COUNT_OUT";
        case BACK_IN: return "BACK_IN";
        case BACK_OUT: return "BACK_OUT";
        default:
            throw std::logic_error("unhandled event: " + std::to_string(e));
    }
}

