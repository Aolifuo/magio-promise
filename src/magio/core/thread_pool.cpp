#include "magio/core/thread_pool.h"

#include "magio/core/logger.h"

namespace magio {

StaticThreadPool::~StaticThreadPool() {
    {
        std::lock_guard lk(mutex_);
        if (state_ != Running) {
            return;
        }
    }
    
    destroy();
}

void StaticThreadPool::start() {
    {
        std::lock_guard lk(mutex_);
        if (state_ != NotStarted) {
            M_FATAL("{}", "You can't start thread pool twice");
        }
        state_ = Running;
    }
    
    std::call_once(once_f_, [&] {
        timer_poller_thread_ = std::thread(&StaticThreadPool::poll_timer_queue, this);
        for (auto& th : threads_) {
            th = std::thread(&StaticThreadPool::run_in_background, this);
        }
    });
    
    cv_.notify_all();
    timer_cv_.notify_one();
}

void StaticThreadPool::destroy() {
    {
        std::lock_guard lk(mutex_);
        if (state_ == PendingDestroy) {
            M_FATAL("{}", "You can't destroy thread pool twice");
        }
        state_ = PendingDestroy;
    }
    cv_.notify_all();
    timer_cv_.notify_one();

    for (auto& th : threads_) {
        if (th.joinable()) {
            th.join();
        }
    }

    if (timer_poller_thread_.joinable()) {
        timer_poller_thread_.join();
    }
}

void StaticThreadPool::post(std::function<void()>&& task) {
    {
        std::lock_guard lk(mutex_);
        tasks_.push_back(std::move(task));
    }
    cv_.notify_one();
}

void StaticThreadPool::run_in_background() {
    std::function<void()> task;
    for (; ;) {
        {
            std::unique_lock lk(mutex_);
            
            cv_.wait(lk, [this] {
                return (state_ == Running && !tasks_.empty()) || state_ == PendingDestroy;
            });
            
            if (state_ == PendingDestroy) {
                M_TRACE("{}", "one thraad function quit");
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop_front();
        }

        try {
            task();
        } catch(...) {
            M_FATAL("{}", "Throw exception when thread function is running");
        }
    }
}

void StaticThreadPool::poll_timer_queue() {
    std::vector<std::function<void(bool)>> expireds;

    for (; ;) {
        expireds.clear();
        
        {
            std::unique_lock lk(timer_m_);

            timer_cv_.wait(lk, [this] {
                return (state_ == Running && !timer_queue_.empty()) || state_ == PendingDestroy;
            });

            if (state_ == PendingDestroy) {
                M_TRACE("{}", "timer poller unction quit");
                return;
            }
            
            timer_queue_.get_expired(expireds);
        }

        for (auto& task : expireds) {
            task(true);
        }
    }
}

}