#ifndef MAGIO_CORE_THREAD_POOL_H_
#define MAGIO_CORE_THREAD_POOL_H_

#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "magio/core/executor.h"
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
        : threads_(thread_num) 
    { }

    ~StaticThreadPool();

    void start();

    void join();

    void post(std::function<void()>&& task) override;

private:
    void run_in_background();

    std::once_flag once_f_;
    std::mutex mutex_;
    std::condition_variable cv_;
    State state_ = NotStarted;
    std::deque<std::function<void()>> tasks_;
    std::vector<std::thread> threads_;
};

}

#endif