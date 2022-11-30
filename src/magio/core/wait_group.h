#ifndef MAGIO_CORE_WAIT_GROUP_H_
#define MAGIO_CORE_WAIT_GROUP_H_

#include <mutex>
#include <condition_variable>

namespace magio {

class WaitGroup {
public:
    WaitGroup(size_t n): wait_n_(n) 
    { }

    void wait() {
        std::unique_lock lk(m_);
        cv_.wait(lk, [this] {
            return wait_n_ == 0;
        });
    }

    void done() {
        std::lock_guard lk(m_);
        --wait_n_;
        if (wait_n_ == 0) {
            cv_.notify_all();
        }
    }
private:
    size_t wait_n_;
    std::mutex m_;
    std::condition_variable cv_;
};

}

#endif