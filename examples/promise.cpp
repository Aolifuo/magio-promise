#include "magio/core/logger.h"
#include "magio/core/promise.h"
#include "magio/core/thread_pool.h"

using namespace std;
using namespace magio;
using namespace chrono_literals;

void promise_chain() {
    StaticThreadPool pool(8);

    Promise::spawn(&pool, [](Defer defer) {
        this_thread::sleep_for(1s);
        M_INFO("{}", "after 1s");
        defer.resolve();
    })->then([&] {
        this_thread::sleep_for(1s);
        M_INFO("{}", "after 1s");
        return Promise::reject(&pool);
    })->then([] {
        this_thread::sleep_for(1s);
        M_INFO("{}", "after 1s");
    })->fail([] {
        M_ERROR("{}", "something error happened");
    });

    pool.start();
    pool.wait_for(3s);
}

void promise_all() {
    StaticThreadPool pool(8);

    vector<PromisePtr> vec {
        Promise::spawn(&pool, [](Defer defer) {
            this_thread::sleep_for(2s);
            M_INFO("{}", "task one completed");
        }),
        Promise::spawn(&pool, [](Defer defer) {
            this_thread::sleep_for(1s);
            M_INFO("{}", "task two completed");
        }),
        Promise::spawn(&pool, [](Defer defer) {
            this_thread::sleep_for(3s);
            M_INFO("{}", "task three completed");
        }),
    };

    Promise::all(&pool, vec)->then([] {
        M_INFO("{}", "all tasks completed");
    })->fail([] {
        M_INFO("{}", "at least one task failed");
    });

    pool.start();
    pool.wait_for(3s);
}

void promise_race() {
    StaticThreadPool pool(8);

    vector<PromisePtr> vec {
        Promise::spawn(&pool, [](Defer defer) {
            this_thread::sleep_for(2s);
            M_INFO("{}", "task one completed");
        }),
        Promise::spawn(&pool, [](Defer defer) {
            this_thread::sleep_for(1s);
            M_INFO("{}", "task two completed");
        }),
        Promise::spawn(&pool, [](Defer defer) {
            this_thread::sleep_for(3s);
            M_INFO("{}", "task three completed");
        }),
    };

    Promise::race(&pool, vec)->then([] {
        M_INFO("{}", "one task complete");
    })->fail([] {
        M_INFO("{}", "at least one task failed");
    });

    pool.start();
    pool.wait_for(3s);
}

void promise_timer() {
    StaticThreadPool pool(8);

    sleep_for(&pool, 3s)->then([] {
        M_INFO("{}", "waker after 3s");
    });

    pool.start();
    pool.wait_for(3s);
}

int main() {
    promise_chain();
    promise_all();
    promise_race();
    promise_timer();

    MAGIO_MEMORY_CHECK;
}