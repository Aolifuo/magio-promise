#ifndef MAGIO_CORE_TRAITS_H_
#define MAGIO_CORE_TRAITS_H_

#include <tuple>

namespace magio {

template<bool Cond>
using constraint = std::enable_if_t<Cond, int>;

// function 
// function noexcept
// function pointer
// function pointer noexcept

// memberfunc pointer
// memberfunc pointer const
// memberfunc pointer noexcept
// memberfunc pointer const noexcept

// Functor

template<typename>
struct Function;

template<typename Ret, typename...Args>
struct Function<Ret(Args...)> {
    using ReturnType = Ret;
    using Params = std::tuple<Args...>;
};

template<typename Functor>
struct Function
    : Function<decltype(&Functor::operator())> { };

template<typename Ret, typename...Args>
struct Function<Ret(Args...) noexcept>
    : Function<Ret(Args...)> { };

template<typename Ret, typename...Args>
struct Function<Ret(*)(Args...)>
    : Function<Ret(Args...)> { };

template<typename Ret, typename...Args>
struct Function<Ret(*)(Args...) noexcept>
    : Function<Ret(Args...)> { };

template<typename Class, typename Ret, typename...Args>
struct Function<Ret(Class::*)(Args...)>
    : Function<Ret(Args...)> { };

template<typename Class, typename Ret, typename...Args>
struct Function<Ret(Class::*)(Args...) const>
    : Function<Ret(Args...)> { };

template<typename Class, typename Ret, typename...Args>
struct Function<Ret(Class::*)(Args...) noexcept>
    : Function<Ret(Args...)> { };

template<typename Class, typename Ret, typename...Args>
struct Function<Ret(Class::*)(Args...) const noexcept>
    : Function<Ret(Args...)> { };

}

#endif