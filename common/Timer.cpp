#include "Timer.h"
#include "Log.h"
using namespace hudp;

inline bool is_between_in(const uint16_t& num, const uint16_t& prev, const uint16_t& next) {
    return num > prev && num <= next;
}

void CTimer::AddTimer(uint16_t ms, CTimerInterface* ti) {
    
}

uint16_t CTimer::RoundedBackwards(uint16_t ms) {
    if (is_between_in(ms, 0, 100)) {
        return ms;
    }
    if (is_between_in(ms, 100, 300)) {
        return RoundedBackwards(ms, 2);
    }

    if (is_between_in(ms, 300, 600)) {
        return ms % 3 + ms;
    }

    if (is_between_in(ms, 600, 1000)) {
        return RoundedBackwards(ms, 4);
    }

    if (is_between_in(ms, 1000, 5000)) {
        return ms % 40 + ms;
    }

    base::LOG_ERROR("timer only support 5s, shouldn't be here.");
    return 0;
}

uint16_t CTimer::RoundedBackwards(uint16_t ms, uint16_t len) {
    return ((ms + len - 1) & ~(len - 1));
}

uint16_t CTimer::GetIndex(uint16_t ms) {
    if (is_between_in(ms, 0, 100)) {
        return ms;
    }
    if (is_between_in(ms, 100, 300)) {
        return 100 + ((ms - 100) >> 1);
    }

    if (is_between_in(ms, 100, 300)) {
        return 200 + ((ms - 300) / 3);
    }

    if (is_between_in(ms, 600, 1000)) {
        return 300 + ((ms - 600) >> 2);
    }

    if (is_between_in(ms, 1000, 5000)) {
        return 400 + ((ms - 1000) / 40);
    }

    base::LOG_ERROR("timer only support 5s, shouldn't be here.");
    return 0;
}
