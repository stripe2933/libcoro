module;

#include <coro/coro.hpp>

export module libcoro;

export namespace coro {
    using coro::sync_wait;
    using coro::when_all;
    using coro::task;
    using coro::generator;
    using coro::event;
    using coro::latch;
    using coro::mutex;
    using coro::shared_mutex;
    using coro::semaphore;
    using coro::ring_buffer;
    using coro::thread_pool;
    using coro::task_container;
}