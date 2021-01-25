#pragma once

#include "coro/event.hpp"

#include <atomic>

namespace coro
{
/**
 * The latch is thread safe counter to wait for 1 or more other tasks to complete, they signal their
 * completion by calling `count_down()` on the latch and upon the latch counter reaching zero the
 * coroutine `co_await`ing the latch then resumes execution.
 *
 * This is useful for spawning many worker tasks to complete either a computationally complex task
 * across a thread pool of workers, or waiting for many asynchronous results like http requests
 * to complete.
 */
class latch
{
public:
    /**
     * Creates a latch with the given count of tasks to wait to complete.
     * @param count The number of tasks to wait to complete, if this is zero or negative then the
     *              latch starts 'completed' immediately and execution is resumed with no suspension.
     */
    latch(std::ptrdiff_t count) noexcept : m_count(count), m_event(count <= 0) {}

    latch(const latch&) = delete;
    latch(latch&&)      = delete;
    auto operator=(const latch&) -> latch& = delete;
    auto operator=(latch&&) -> latch& = delete;

    /**
     * @return True if the latch has been counted down to zero.
     */
    auto is_ready() const noexcept -> bool { return m_event.is_set(); }

    /**
     * @return The number of tasks this latch is still waiting to complete.
     */
    auto remaining() const noexcept -> std::size_t { return m_count.load(std::memory_order::acquire); }

    /**
     * @param n The number of tasks to complete towards the latch, defaults to 1.
     */
    auto count_down(std::ptrdiff_t n = 1) noexcept -> void
    {
        if (m_count.fetch_sub(n, std::memory_order::acq_rel) <= n)
        {
            m_event.set();
        }
    }

    auto operator co_await() const noexcept -> event::awaiter { return m_event.operator co_await(); }

private:
    /// The number of tasks to wait for completion before triggering the event to resume.
    std::atomic<std::ptrdiff_t> m_count;
    /// The event to trigger when the latch counter reaches zero, this resume the coroutine that
    /// is co_await'ing on the latch.
    event m_event;
};

} // namespace coro
