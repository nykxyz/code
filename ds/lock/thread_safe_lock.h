// 适用于极致性能场景的自旋锁实现
#include <atomic>
#include <thread>

class SpinLock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock() noexcept {
        while (flag.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }
    void unlock() noexcept {
        flag.clear(std::memory_order_release);
    }
    bool try_lock() noexcept {
        return !flag.test_and_set(std::memory_order_acquire);
    }
};

// 读写自旋锁（简单实现，适合读多写少场景）
class SpinSharedMutex {
    std::atomic<int> readers{0};
    std::atomic_flag writer = ATOMIC_FLAG_INIT;
public:
    void lock() noexcept {
        while (writer.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        while (readers.load(std::memory_order_acquire) > 0) {
            std::this_thread::yield();
        }
    }
    void unlock() noexcept {
        writer.clear(std::memory_order_release);
    }
    bool try_lock() noexcept {
        if (!writer.test_and_set(std::memory_order_acquire)) {
            if (readers.load(std::memory_order_acquire) == 0) {
                return true;
            } else {
                writer.clear(std::memory_order_release);
            }
        }
        return false;
    }
    void lock_shared() noexcept {
        for (;;) {
            while (writer.test(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            readers.fetch_add(1, std::memory_order_acquire);
            if (!writer.test(std::memory_order_acquire)) break;
            readers.fetch_sub(1, std::memory_order_release);
        }
    }
    void unlock_shared() noexcept {
        readers.fetch_sub(1, std::memory_order_release);
    }
    bool try_lock_shared() noexcept {
        if (!writer.test(std::memory_order_acquire)) {
            readers.fetch_add(1, std::memory_order_acquire);
            if (!writer.test(std::memory_order_acquire)) {
                return true;
            } else {
                readers.fetch_sub(1, std::memory_order_release);
            }
        }
        return false;
    }
};
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