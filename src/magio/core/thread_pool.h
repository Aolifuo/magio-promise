#ifndef MAGIO_CORE_THREAD_POOL_H_
#define MAGIO_CORE_THREAD_POOL_H_

#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "magio/core/executor.h"
#include "magio/core/timer_queue.h"
#include "magio/core/noncopyable.h"

namespace magio {

class StaticThreadPool final: Noncopyable, public Executor {
public:
    enum State {
        NotStarted,
        Running,
        PendingDestroy
    };

    StaticThreadPool(size_t thread_num)
        : threads_(thread_num + 1) 
    { }

    ~StaticThreadPool();

    void start();

    template<typename Rep, typename Per>
    void wait_for(const std::chrono::duration<Rep, Per>& dur) {
        std::this_thread::sleep_for(dur);
    }

    template<typename Clock, typename Dur>
    void wait_until(const std::chrono::time_point<Clock, Dur>& tp) {
        std::this_thread::sleep_until(tp);
    }

    void destroy();

    void post(std::function<void()>&& task) override;

    template<typename Rep, typename Per>
    void expires_after(const std::chrono::duration<Rep, Per>& dur, std::function<void(bool)>&& task) {
        {
            std::lock_guard lk(timer_m_);
            timer_queue_.push(TimerClock::now() + dur, std::move(task));
        }
        timer_cv_.notify_one();
    }

    void expires_until(const TimerClock::time_point& tp, std::function<void(bool)>&& task) {
        {
            std::lock_guard lk(timer_m_);
            timer_queue_.push(tp, std::move(task));
        }
        timer_cv_.notify_one();
    }

private:
    void run_in_background();

    void poll_timer_queue();

    std::once_flag once_f_;

    std::atomic<State> state_ = NotStarted;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> tasks_;

    std::mutex timer_m_;
    std::condition_variable timer_cv_;
    TimerQueue timer_queue_;

    std::vector<std::thread> threads_;
    std::thread timer_poller_thread_;
};

}

#endif