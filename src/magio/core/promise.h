#ifndef MAGIO_CORE_PROMISE_H_
#define MAGIO_CORE_PROMISE_H_

#include <mutex>
#include <memory>
#include <atomic>

#include "magio/core/traits.h"
#include "magio/core/executor.h"
#include "magio/core/noncopyable.h"
#include "magio/dev/memory_check.h"

namespace magio {

class Promise;

using PromisePtr = std::shared_ptr<Promise>;

class Defer {
    friend class Promise;

    Defer (const std::shared_ptr<Promise>& promise)
        : promise_(promise) { }

public:
    void resolve() const;

    void reject() const;

private:
    std::shared_ptr<Promise> promise_;
};

class Promise: public std::enable_shared_from_this<Promise> {
    friend class Defer;
    
    Promise(Executor* executor)
        : executor_(executor) { }

public:
    enum State {
        Pending,
        Resolved,
        Rejected
    };

    static PromisePtr spawn(Executor* executor, std::function<void(Defer)>&& fn) {
        MAGIO_NEW_PROMISE;
        std::shared_ptr<Promise> ptr(new Promise(executor), 
            [](Promise* p) {
                MAGIO_DESTROY_PROMISE;
                delete p;
            });

        executor->post([ptr, fn = std::move(fn)] {
            fn(Defer(ptr));
        });
        return ptr;
    }

    static PromisePtr resolve(Executor* executor) {
        return spawn(executor, [](Defer defer) {
            defer.resolve();
        });
    }

    static PromisePtr reject(Executor* executor) {
        return spawn(executor, [](Defer defer) {
            defer.reject();
        });
    }

    template<
        typename Range,
        constraint<
            IsRange<Range>::value &&
            std::is_same_v<PromisePtr, typename RangeTraits<Range>::ValueType>
        > = 0
    >
    static PromisePtr all(Executor* executor, const Range& range) {
        return sync_spawn(executor, [&range](Defer defer) {
            auto counter = std::make_shared<std::atomic_size_t>(0);

            for (auto& ptr : range) {
                counter->fetch_add(1);
                ptr->then([counter, defer] {
                    counter->fetch_sub(1);
                    if (counter == 0) {
                        defer.resolve();
                    }
                }, [defer] {
                    defer.reject();
                });
            }
        });
    }

    template<
        typename Range,
        constraint<
            IsRange<Range>::value &&
            std::is_same_v<PromisePtr, typename RangeTraits<Range>::ValueType>
        > = 0
    >
    static PromisePtr race(Executor* executor, const Range& range) {
        return sync_spawn(executor, [&range](Defer defer) {
            for (auto& ptr : range) {
                ptr->then([defer] {
                    defer.resolve();
                }, [defer] {
                    defer.reject();
                });
            }
        });
    }

    template<typename OnResolved, typename OnRejected>
    PromisePtr then(OnResolved on_resolved, OnRejected on_rejected) {
        auto new_promise = sync_spawn(
            executor_, 
            [
                ptr = shared_from_this(),
                on_resolved = std::move(on_resolved),
                on_rejected = std::move(on_rejected)
            ] (Defer defer) mutable {
                std::lock_guard lk(ptr->mutex_);
                ptr->resolve_fns_.emplace_back(func_impl(defer, std::move(on_resolved), true));
                ptr->reject_fns_.emplace_back(func_impl(defer, std::move(on_rejected), true));
            }
        );

        return new_promise;
    }

    template<typename OnResolved>
    PromisePtr then(OnResolved on_resolved) {
        auto new_promise = sync_spawn(
            executor_, 
            [
                ptr = shared_from_this(),
                on_resolved = std::move(on_resolved)
            ](Defer defer) mutable {
                ptr->resolve_fns_.emplace_back(func_impl(defer, std::move(on_resolved), true));
                ptr->reject_fns_.emplace_back(func_impl(defer, [] {}, false));
            }
        );

        return new_promise;
    }

    template<typename OnRejected>
    PromisePtr fail(OnRejected on_rejected) {
        auto new_promise = sync_spawn(
            executor_, 
            [
                ptr = shared_from_this(),
                on_rejected = std::move(on_rejected)
            ](Defer defer) mutable {
                std::lock_guard lk(ptr->mutex_);
                ptr->resolve_fns_.emplace_back(func_impl(defer, [] {}, true));
                ptr->reject_fns_.emplace_back(func_impl(defer, std::move(on_rejected), true));
            }
        );

        return new_promise;
    }

    State state() {
        return state_;
    }

private:
    static PromisePtr sync_spawn(Executor* executor, std::function<void(Defer)>&& fn) {
        MAGIO_NEW_PROMISE;
        std::shared_ptr<Promise> ptr(new Promise(executor), 
            [](Promise* p) {
                MAGIO_DESTROY_PROMISE;
                delete p;
            });

        fn(Defer(ptr));
        return ptr;
    }

    template<typename Fn>
    static auto func_impl(const Defer& defer, Fn&& fn, bool flag) {
        return [
            defer,
            fn = std::forward<Fn>(fn),
            flag
        ]() mutable {
            if constexpr(std::is_same_v<PromisePtr, std::invoke_result_t<std::decay_t<Fn>>>) {
                auto ptr = fn();
                ptr->then(
                    [defer] {
                        defer.resolve();
                    },
                    [defer] {
                        defer.reject();
                    });
            } else {
                fn();
                if (flag) {
                    defer.resolve();
                } else {
                    defer.reject();
                }
            }
        };
    }

    void resolve_impl() {
        std::lock_guard lk(mutex_);
        if (state_ != Pending) {
            return;
        }
        state_ = Resolved;

        for (auto& fn : resolve_fns_) {
            executor_->post(std::move(fn));
        }
    }

    void reject_impl() {
        std::lock_guard lk(mutex_);
        if (state_ != Pending) {
            return;
        }
        state_ = Rejected;
        
        for (auto& fn : reject_fns_) {
            executor_->post(std::move(fn));
        }
    }

    std::mutex mutex_;

    Executor* executor_;
    State state_ = Pending;
    std::exception_ptr eptr_;

    std::vector<std::function<void()>> resolve_fns_;
    std::vector<std::function<void()>> reject_fns_;
};

void Defer::resolve() const {
    promise_->resolve_impl();
}

void Defer::reject() const {
    promise_->reject_impl();
}

// timer

template<typename Exe, typename Rep, typename Per>
PromisePtr sleep_for(Exe* exe, const std::chrono::duration<Rep, Per>& dur) {
    return Promise::spawn(exe, [dur, exe](Defer defer) {
        exe->expires_after(dur, [defer](bool) {
            defer.resolve();
        });
    });
}

template<typename Exe>
PromisePtr sleep_until(Exe* exe, const std::chrono::steady_clock::time_point& tp) {
    return Promise::spawn(exe, [tp, exe](Defer defer) {
        exe->expires_after(tp, [defer](bool) {
            defer.resolve();
        });
    });
}

}

#endif