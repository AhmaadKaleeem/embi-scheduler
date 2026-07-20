/**
 * @file LockManager.hpp
 * @brief Pool of discrete simulation locks with FIFO waiter queues.
 *
 * LockManager is a pure data structure — it records ownership and waiter
 * queues for each lock in the pool.  It does NOT modify Process state;
 * that responsibility belongs to EventLoop, which calls Process::arrival()
 * and Process::service() based on LockManager output.
 *
 * This separation is deliberate: it keeps the fluid-model updates in one
 * place (EventLoop) and makes LockManager easily unit-testable.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

namespace embi {

/**
 * @class LockManager
 * @brief Fixed-size pool of simulation locks with FIFO wait queues.
 *
 * @par Threading
 * Not thread-safe — each EventLoop owns its own LockManager instance.
 *
 * @par Example
 * @code
 * embi::LockManager mgr(4);       // 4 locks
 * bool ok = mgr.acquire(0, 7);    // PID 7 acquires lock 0 → true (free)
 * bool ok2 = mgr.acquire(0, 3);   // PID 3 tries lock 0 → false (queued)
 * int  next = mgr.release(0);     // PID 7 releases → returns 3
 * @endcode
 */
class LockManager {
public:
    /**
     * @brief Constructs a LockManager with `num_locks` initially free locks.
     * @param num_locks  Number of independent locks in the pool (≥ 1).
     */
    explicit LockManager(std::size_t num_locks);

    // ─── Acquire / Release ────────────────────────────────────────────────────

    /**
     * @brief Attempts to acquire a lock on behalf of a process.
     *
     * - If the lock is free, `pid` becomes the owner and the function
     *   returns `true`.
     * - If the lock is held, `pid` is appended to the FIFO wait queue
     *   and the function returns `false`.
     *
     * @param lock_id  Index into the lock pool, must be < numLocks().
     * @param pid      Process ID requesting the lock.
     * @return `true` if immediately acquired; `false` if queued.
     */
    bool acquire(std::size_t lock_id, std::size_t pid);

    /**
     * @brief Releases a lock and promotes the head of the wait queue.
     *
     * The current owner gives up the lock.  If there are waiting processes,
     * the next one becomes the owner and is returned.  The caller is
     * responsible for giving that process its new LockState (hold duration,
     * etc.).
     *
     * @param lock_id  Index into the lock pool.
     * @return  PID of the newly promoted owner, or -1 if the queue was empty.
     * @pre  lock_id must currently be held (owner >= 0).
     */
    int release(std::size_t lock_id);

    // ─── Queries ──────────────────────────────────────────────────────────────

    /// Returns true if the lock has no current owner.
    [[nodiscard]] bool isFree(std::size_t lock_id) const noexcept;

    /// Returns the PID of the current owner, or -1 if free.
    [[nodiscard]] int owner(std::size_t lock_id) const noexcept;

    /// Returns the number of processes waiting (not including the owner).
    [[nodiscard]] std::size_t waiterCount(std::size_t lock_id) const noexcept;

    /// Returns the total number of locks in the pool.
    [[nodiscard]] std::size_t numLocks() const noexcept;

    /// Resets all locks to free with empty waiter queues.
    void reset();

private:
    struct LockState {
        int                      owner{-1};   ///< Current owner PID, or -1 if free.
        std::queue<std::size_t>  waiters;     ///< FIFO wait queue.
    };

    std::vector<LockState> locks_;
};

} // namespace embi
