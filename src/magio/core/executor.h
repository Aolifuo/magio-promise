#ifndef MAGIO_CORE_EXECUTOR_H_
#define MAGIO_CORE_EXECUTOR_H_

#include <functional>

namespace magio {

// Interface
class Executor {
public:
    virtual ~Executor() = default;

    virtual void post(std::function<void()>&&) = 0;
private:
};

}

#endif