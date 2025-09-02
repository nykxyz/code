// 线程安全锁策略：空锁实现，便于泛型数据结构零开销
#pragma once

struct NullMutex {
    void lock() noexcept {}
    void unlock() noexcept {}
    bool try_lock() noexcept { return true; }
};

struct NullSharedMutex : NullMutex {
    void lock_shared() noexcept {}
    void unlock_shared() noexcept {}
    bool try_lock_shared() noexcept { return true; }
};