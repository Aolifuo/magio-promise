#ifndef MAGIO_CORE_PROMISE_H_
#define MAGIO_CORE_PROMISE_H_

#include <mutex>
#include <memory>

#include "magio/core/traits.h"
#include "magio/core/executor.h"
#include "magio/core/noncopyable.h"
#include "magio/dev/memory_check.h"

namespace magio {

class Promise;
class Resolve;
class Reject;

using PromisePtr = std::shared_ptr<Promise>;

class Resolve {
    friend class Promise;

    Resolve(const std::shared_ptr<Promise>& promise)
        : promise_(promise) { }

public:
    void operator()() const;

    auto get_weak() {
        return promise_;
    }

private:
    std::weak_ptr<Promise> promise_;
};

class Reject {
    friend class Promise;

    Reject(const std::shared_ptr<Promise>& promise)
        : promise_(promise) { }

public:
    void operator()() const;

    auto get_weak() {
        return promise_;
    }

private:
    std::weak_ptr<Promise> promise_;
};

class Promise: public std::enable_shared_from_this<Promise> {
    friend class Resolve;
    friend class Reject;
    
    Promise(Executor* executor)
        : executor_(executor) { }

public:
    enum State {
        Pending,
        Resolved,
        Rejected
    };

    static PromisePtr spawn(Executor* executor, std::function<void(Resolve, Reject)>&& fn) {
        MAGIO_NEW_PROMISE;
        std::shared_ptr<Promise> ptr(new Promise(executor), 
            [](Promise* p) {
                MAGIO_DESTROY_PROMISE;
                delete p;
            });

        executor->post([ptr, fn = std::move(fn)] {
            fn(Resolve(ptr), Reject(ptr));
        });
        return ptr;
    }

    template<
        typename OnResolved, 
        typename OnRejected,
        constraint<
            std::is_invocable_v<OnResolved> &&
            std::is_invocable_v<OnRejected>
        > = 0
    >
    PromisePtr then(
        OnResolved&& on_resolved,
        OnRejected&& on_rejected
    ) {
        auto new_promise = spawn(
            executor_, 
            [
                ptr = shared_from_this(),
                on_resolved = std::forward<OnResolved>(on_resolved),
                on_rejected = std::forward<OnRejected>(on_rejected)
            ] (Resolve resolve, Reject reject) mutable {
                ptr->resolve_fns_.emplace_back(
                    [
                        span_life = resolve.get_weak().lock(),
                        resolve = std::move(resolve),
                        on_resolved = std::move(on_resolved)
                    ]() mutable {
                        if constexpr(std::is_same_v<PromisePtr, std::invoke_result_t<OnResolved>>) {
                            auto ptr = on_resolved();
                            ptr->then([resolve = std::move(resolve)] {
                                resolve();
                            });
                        } else {
                            on_resolved();
                            resolve();
                        }
                    }
                );

                ptr->reject_fns_.emplace_back(
                    [
                        span_life = reject.get_weak().lock(),
                        reject = std::move(reject),
                        on_rejected = std::move(on_rejected)
                    ]() mutable {
                        if constexpr(std::is_same_v<PromisePtr, std::invoke_result_t<OnRejected>>) {
                            auto ptr = on_rejected();
                            ptr->then([reject = std::move(reject)] {
                                reject();
                            });
                        } else {
                            on_rejected();
                            reject();
                        }
                    }
                );
            }
        );

        return new_promise;
    }

    template<
        typename OnResolved, 
        constraint<std::is_invocable_v<OnResolved>> = 0
    >
    PromisePtr then(OnResolved&& on_resolved) {
        return then(std::forward<OnResolved>(on_resolved), nullptr);
    }

    template<
        typename OnRejected,
        constraint<std::is_invocable_v<OnRejected, std::exception_ptr>> = 0
    >
    PromisePtr catch_error(OnRejected&& on_rejected);

    State state() {
        return state_;
    }

private:
    void resolve() {
        {
            std::lock_guard lk(mutex_);
            if (state_ != Pending) {
                return;
            }
            state_ = Resolved;
        }

        for (auto& fn : resolve_fns_) {
            executor_->post(std::move(fn));
        }
    }

    void reject() {
        {
            std::lock_guard lk(mutex_);
            if (state_ != Pending) {
                return;
            }
            state_ = Rejected;
        }
        
        for (auto& fn : reject_fns_) {
            if (fn) {
                executor_->post(std::move(fn));
            } else {
                
            }
        }

        executor_->post([eptr = eptr_, fn = std::move(handle_except_)] {
            fn(eptr);
        });
    }

    std::mutex mutex_;

    Executor* executor_;
    State state_ = Pending;
    std::exception_ptr eptr_;

    std::vector<std::function<void()>> resolve_fns_;
    std::vector<std::function<void()>> reject_fns_;
    std::function<void(std::exception_ptr)> handle_except_;
};

void Resolve::operator()() const {
    auto ptr = promise_.lock();
    if (ptr) {
        ptr->resolve();
    }
}

void Reject::operator()() const {
    auto ptr = promise_.lock();
    if (ptr) {
        ptr->reject();
    }
}

}

#endif