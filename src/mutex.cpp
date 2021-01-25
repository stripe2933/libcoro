#include "coro/mutex.hpp"

namespace coro
{
auto mutex::lock() -> awaiter
{
    return awaiter(*this);
}

auto mutex::try_lock() -> bool
{
    bool expected = false;
    return m_state.compare_exchange_strong(expected, true, std::memory_order::release, std::memory_order::relaxed);
}

auto mutex::unlock() -> void
{
    // Get the next waiter before releasing the lock.
    awaiter* next{nullptr};
    {
        std::scoped_lock lk{m_waiter_mutex};
        if (!m_waiter_list.empty())
        {
            next = m_waiter_list.front();
            m_waiter_list.pop_front();
        }
    }

    // Unlock the mutex
    m_state.exchange(false, std::memory_order::release);

    // If there was a awaiter, resume it.  Here would be good place to _resume_ the waiter onto
    // the thread pool to distribute the work, this currently implementation will end up having
    // every waiter on the mutex jump onto a single thread.
    if (next != nullptr)
    {
        next->m_awaiting_coroutine.resume();
    }
}

auto mutex::awaiter::await_ready() const noexcept -> bool
{
    return m_mutex.try_lock();
}

auto mutex::awaiter::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool
{
    m_awaiting_coroutine = awaiting_coroutine;

    {
        // Its possible between await_ready() and await_suspend() the lock was released,
        // if thats the case acquire it immediately.
        std::scoped_lock lk{m_mutex.m_waiter_mutex};
        if (m_mutex.m_waiter_list.empty() && m_mutex.try_lock())
        {
            return false;
        }

        // Ok its still held, add ourself to the waiter list.
        m_mutex.m_waiter_list.emplace_back(this);
    }

    // The mutex is still locked and we've added this to the waiter list, suspend now.
    return true;
}

auto mutex::awaiter::await_resume() noexcept -> scoped_lock
{
    return scoped_lock{m_mutex};
}

} // namespace coro
