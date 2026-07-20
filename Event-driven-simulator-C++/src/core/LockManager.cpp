/**
 * @file LockManager.cpp
 * @brief Implementation of LockManager.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "core/LockManager.hpp"

#include <stdexcept>

namespace embi {

LockManager::LockManager(std::size_t num_locks) {
    if (num_locks == 0) {
        throw std::invalid_argument("LockManager: num_locks must be >= 1");
    }
    locks_.resize(num_locks);
}

bool LockManager::acquire(std::size_t lock_id, std::size_t pid) {
    LockState& ls = locks_[lock_id];
    if (ls.owner < 0) {
        // Lock is free — grant immediately.
        ls.owner = static_cast<int>(pid);
        return true;
    }
    // Lock is held — queue the waiter.
    ls.waiters.push(pid);
    return false;
}

int LockManager::release(std::size_t lock_id) {
    LockState& ls = locks_[lock_id];
    if (ls.waiters.empty()) {
        // No waiters — lock becomes free.
        ls.owner = -1;
        return -1;
    }
    // Promote the head-of-queue waiter to owner.
    int next_owner = static_cast<int>(ls.waiters.front());
    ls.waiters.pop();
    ls.owner = next_owner;
    return next_owner;
}

bool LockManager::isFree(std::size_t lock_id) const noexcept {
    return locks_[lock_id].owner < 0;
}

int LockManager::owner(std::size_t lock_id) const noexcept {
    return locks_[lock_id].owner;
}

std::size_t LockManager::waiterCount(std::size_t lock_id) const noexcept {
    return locks_[lock_id].waiters.size();
}

std::size_t LockManager::numLocks() const noexcept {
    return locks_.size();
}

void LockManager::reset() {
    for (auto& ls : locks_) {
        ls.owner = -1;
        while (!ls.waiters.empty()) ls.waiters.pop();
    }
}

} // namespace embi
