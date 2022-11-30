#ifndef MAGIO_CORE_TIMER_QUEUE_H
#define MAGIO_CORE_TIMER_QUEUE_H

#include <queue>
#include <atomic>
#include <memory>
#include <chrono>
#include <functional>

#include "magio/core/logger.h"

namespace magio {

using TimerClock = std::chrono::steady_clock;

struct TimerData {
    TimerData(TimerClock::time_point tp, std::function<void(bool)>&& f)
        : dead_line(tp), task(std::move(f)) 
    { }

    TimerClock::time_point dead_line;
    std::function<void(bool)> task;
};

struct TimerCompare {
    bool operator()(
        const TimerData& left, 
        const TimerData& right
    ) const {
        return left.dead_line < right.dead_line;
    }
};

class TimerQueue {
    using QueueType = std::priority_queue<
        TimerData,
        std::vector<TimerData>,
        TimerCompare
    >;

public:
    bool get_expired(std::vector<std::function<void(bool)>>& res) {
        auto current_tp = TimerClock::now();

        for (; !timers_.empty() && current_tp >= timers_.top().dead_line;) {
            res.push_back(std::move(const_cast<std::function<void(bool)>&>(timers_.top().task)));
            timers_.pop();
        }

        return !res.empty();
    }

    void push(const TimerClock::time_point& tp, std::function<void(bool)>&& task) {
        timers_.emplace(tp, std::move(task));
    }

    size_t empty() {
        return timers_.empty();
    }

private:
    QueueType timers_;

};

}

#endif