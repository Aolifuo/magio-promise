#ifndef MAGIO_CORE_EXECUTOR_H_
#define MAGIO_CORE_EXECUTOR_H_

#include <functional>

namespace magio {

// Interface
class Executor {
public:
    virtual ~Executor() = default;

    virtual void post(std::function<void()>&&) = 0;

    // template<typename Rep, typename Per>
    // void expires_after(const std::chrono::duration<Rep, Per>& dur, std::function<void(bool)>&&);

    // void expires_until(const std::chrono::steady_clock::time_point& tp, std::function<void(bool)>&&);

    // void cancel();
    
private:
};

}

#endif