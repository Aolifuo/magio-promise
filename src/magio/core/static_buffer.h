#ifndef MAGIO_CORE_STATIC_BUFFER_H_
#define MAGIO_CORE_STATIC_BUFFER_H_

#include <string>

#include "fmt/core.h"

namespace magio {

constexpr size_t kFormatBufferSize = 500;
constexpr size_t kSmallBufferSize = 4000;
constexpr size_t kLargeBufferSize = 4000 * 1000;

template<size_t Size>
class StaticBuffer {
public:
    size_t append(std::string_view msg) {
        size_t cplen = std::min(rest(), msg.length());
        memcpy(buf_ + size_, msg.data(), cplen);
        size_ += cplen;
        return cplen;
    }

    size_t append_format(std::string_view fmt, fmt::format_args args) {
        auto sv = fmt::vformat_to_n(buf_ + size_, rest(), fmt, args);
        size_ += sv.size;
        return sv.size;
    }

    size_t rest() {
        return Size - size_;
    }

    void clear() {
        size_ = 0;
    }
    
    size_t size() {
        return size_;
    }

    std::string_view str_view() {
        return {buf_, size_};
    }

private:
    char buf_[Size];
    size_t size_ = 0;
};

namespace detail {

using FormatBuffer = StaticBuffer<kFormatBufferSize>;
using SmallBuffer = StaticBuffer<kSmallBufferSize>;
using LargeBuffer = StaticBuffer<kLargeBufferSize>;

}

}

#endif