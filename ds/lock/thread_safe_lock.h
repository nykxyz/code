#ifndef THREAD_SAFE_LOCK_H
#define THREAD_SAFE_LOCK_H
#include <vector>
#include <shared_mutex>
#include <functional>
#include <cstddef>
#include <mutex>
template <size_t NumStripes = 64, typename UnderlyingMutex = std::shared_mutex>
class StripedSharedMutex {
public:
    StripedSharedMutex() : stripes_(NumStripes) {}
    
    // 避免全局锁，这些方法实际上很少使用
    void lock() = delete;
    void unlock() = delete; 
    void lock_shared() = delete;
    void unlock_shared() = delete;
    bool try_lock() = delete;
    bool try_lock_shared() = delete;

    // 核心功能：获取分段锁
    UnderlyingMutex& stripe(size_t i) noexcept { 
        return stripes_[i % NumStripes]; 
    }
    UnderlyingMutex& stripe(size_t i) const noexcept {
        return const_cast<UnderlyingMutex&>(stripes_[i % NumStripes]);
    }
    
    constexpr size_t stripes() const noexcept { return NumStripes; }

    // 优化：更好的哈希分布
    template<typename Key>
    size_t stripe_index(const Key& key) const noexcept {
        // 使用更好的哈希混合
        auto hash = std::hash<Key>{}(key);
        hash ^= hash >> 16;  // 混合高位和低位
        return hash % NumStripes;
    }

private:
    alignas(64) std::vector<UnderlyingMutex> stripes_;  // 缓存行对齐
};


class SpinSharedMutex {
    static constexpr uint32_t WRITER_MASK = 0x80000000;
    static constexpr uint32_t READER_MASK = 0x7FFFFFFF;
    
    alignas(64) std::atomic<uint32_t> state_{0};  // 缓存行对齐
    
public:
    void lock() noexcept {
        uint32_t expected = 0;
        while (!state_.compare_exchange_weak(expected, WRITER_MASK, 
                                           std::memory_order_acquire)) {
            expected = 0;
            for (int i = 0; i < 16; ++i) {
                if (state_.load(std::memory_order_relaxed) == 0) break;
                std::this_thread::yield();
            }
        }
    }
    
    void unlock() noexcept {
        state_.store(0, std::memory_order_release);
    }
    
    bool try_lock() noexcept {
        uint32_t expected = 0;
        return state_.compare_exchange_strong(expected, WRITER_MASK, 
                                            std::memory_order_acquire);
    }
    
    void lock_shared() noexcept {
        uint32_t current = state_.load(std::memory_order_relaxed);
        while (true) {
            // 等待写锁释放
            while (current & WRITER_MASK) {
                std::this_thread::yield();
                current = state_.load(std::memory_order_relaxed);
            }
            
            if (state_.compare_exchange_weak(current, current + 1, 
                                           std::memory_order_acquire)) {
                break;
            }
        }
    }
    
    void unlock_shared() noexcept {
        state_.fetch_sub(1, std::memory_order_release);
    }
    
    bool try_lock_shared() noexcept {
        uint32_t current = state_.load(std::memory_order_relaxed);
        return !(current & WRITER_MASK) && 
               state_.compare_exchange_strong(current, current + 1, 
                                            std::memory_order_acquire);
    }
};

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

#endif // THREAD_SAFE_LOCK_H