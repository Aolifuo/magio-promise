# Magio Promise

C++ Implementation of Javascript Promise

## Features

Todo

### Sample code 1

Promise chain

```cpp
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
```

output

```shell
info 2022-11-30 19:10:03 f:examples/promise.cpp l:14 id:140420743948032 after 1s
info 2022-11-30 19:10:04 f:examples/promise.cpp l:18 id:140420743948032 after 1s
error 2022-11-30 19:10:04 f:examples/promise.cpp l:24 id:140420710377216 something error happened
```

### Sample code 2

```cpp
void promise_all_and_race() {
    StaticThreadPool pool(8);

    vector<PromisePtr> vec {
        Promise::spawn(&pool, [](Defer defer) {
            this_thread::sleep_for(2s);
            M_INFO("{}", "task one completed");
            defer.resolve();
        }),
        Promise::spawn(&pool, [](Defer defer) {
            this_thread::sleep_for(1s);
            M_INFO("{}", "task two completed");
            defer.resolve();
        }),
        Promise::spawn(&pool, [](Defer defer) {
            this_thread::sleep_for(3s);
            M_INFO("{}", "task three completed");
            defer.resolve();
        }),
    };

    Promise::all(&pool, vec)->then([] {
        M_INFO("{}", "all tasks completed");
    })->fail([] {
        M_INFO("{}", "at least one task failed");
    });

    // or Promise::race
    // Promise::race(&pool, vec)->then([] {
    //     M_INFO("{}", "one task complete");
    // })->fail([] {
    //     M_INFO("{}", "at least one task failed");
    // });

    pool.start();
    pool.wait_for(3s);
}
```

```shell
info 2022-11-30 19:10:06 f:examples/promise.cpp l:42 id:140420685199104 task two completed
info 2022-11-30 19:10:07 f:examples/promise.cpp l:37 id:140420676806400 task one completed
info 2022-11-30 19:10:08 f:examples/promise.cpp l:47 id:140420693591808 task three completed
info 2022-11-30 19:10:08 f:examples/promise.cpp l:53 id:140420735555328 all tasks completed
```

### Sample code 3

```cpp
void promise_timer() {
    StaticThreadPool pool(8);

    sleep_for(&pool, 3s)->then([] {
        M_INFO("{}", "waker after 3s");
    });

    pool.start();
    pool.wait_for(3s);
}
```

```shell
info 2022-11-30 19:10:16 f:examples/promise.cpp l:97 id:140420701984512 waker after 3s
```
