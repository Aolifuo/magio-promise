#ifndef MAGIO_CORE_CURRENT_THREAD_H_
#define MAGIO_CORE_CURRENT_THREAD_H_

#include <thread>
#include <sstream>

namespace magio {

class CurrentThread {
public:
    static std::string_view get_id() {
        static thread_local std::string id = [] {
            std::ostringstream ss;
            ss << std::this_thread::get_id();
            return std::string(ss.str());
        }();
        return id;
    }

private:
};

}

#endif