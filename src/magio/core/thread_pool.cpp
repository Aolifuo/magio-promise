#include "magio/core/thread_pool.h"

namespace magio {

StaticThreadPool::~StaticThreadPool() {
    {
        std::lock_guard lk(mutex_);
        if (state_ == NotStarted) {
            return;
        }
    }
    
    join();
}

void StaticThreadPool::start() {
        std::call_once(once_f_, [&] {
            for (auto& th : threads_) {
                th = std::thread(&StaticThreadPool::run_in_background, this);
            }
        });

        {
            std::lock_guard lk(mutex_);
            state_ = Running;
        }
        cv_.notify_all();
    }

void StaticThreadPool::join() {
    {
        std::lock_guard lk(mutex_);
        state_ = PendingDestroy;
    }
    cv_.notify_all();

    for (auto& th : threads_) {
        if (th.joinable()) {
            th.join();
        }
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
            
            if (state_ == PendingDestroy && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop_front();
        }

        task();
    }
}

}