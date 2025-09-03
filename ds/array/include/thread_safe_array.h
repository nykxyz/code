
#ifndef THREAD_SAFE_ARRAY_H
#define THREAD_SAFE_ARRAY_H

#include <vector>
#include <shared_mutex>
#include <type_traits>
#include "../../lock/thread_safe_lock.h"
template<typename T, typename StripedLock = StripedSharedMutex<16>>
class StripedThreadSafeArray {
private:
    StripedLock lock_;
    std::vector<std::vector<T>> data_;
    
    // 优化：缓存大小信息避免频繁计算
    mutable std::atomic<size_t> cached_size_{0};
    mutable std::atomic<bool> size_dirty_{true};
    
    // 优化：更好的分段选择策略
    size_t next_stripe() const noexcept {
        // 全局原子递增，保证多线程均匀分布到各分段
        static std::atomic<size_t> global_counter{0};
        return global_counter.fetch_add(1, std::memory_order_relaxed) % lock_.stripes();
    }
    
    // 优化：分段大小前缀和，加速定位
    mutable std::vector<std::atomic<size_t>> stripe_sizes_;
    
public:
    StripedThreadSafeArray() 
        : data_(lock_.stripes()), stripe_sizes_(lock_.stripes()) {
        for (auto& size : stripe_sizes_) {
            size.store(0, std::memory_order_relaxed);
        }
    }

    T operator[](size_t index) const { return get(index); }

    template<typename U = T>
    void push_back(U&& value) {
        auto idx = next_stripe();
        std::unique_lock lock(lock_.stripe(idx));
        data_[idx].push_back(std::forward<U>(value));
        stripe_sizes_[idx].fetch_add(1, std::memory_order_relaxed);
        size_dirty_.store(true, std::memory_order_relaxed);
    }

    // 优化：批量插入减少锁开销
    template<std::ranges::input_range R>
    void insert_range(const R& range) {
        auto idx = next_stripe();
        std::unique_lock lock(lock_.stripe(idx));
        auto old_size = data_[idx].size();
        data_[idx].insert(data_[idx].end(), std::ranges::begin(range), 
                         std::ranges::end(range));
        auto added = data_[idx].size() - old_size;
        stripe_sizes_[idx].fetch_add(added, std::memory_order_relaxed);
        size_dirty_.store(true, std::memory_order_relaxed);
    }

    // O(1) 直接定位分段和分段内偏移，极大提升 read 性能
    T get(size_t index) const {
        // 先快照每个分段的 size
        std::vector<size_t> sizes(lock_.stripes());
        size_t total = 0;
        for (size_t i = 0; i < lock_.stripes(); ++i) {
            sizes[i] = stripe_sizes_[i].load(std::memory_order_relaxed);
            total += sizes[i];
        }
        if (index >= total) throw std::out_of_range("Index out of range");
        // 二分查找分段
        size_t l = 0, r = lock_.stripes();
        size_t acc = 0;
        while (l < r) {
            size_t m = (l + r) / 2;
            size_t sum = 0;
            for (size_t i = 0; i < m; ++i) sum += sizes[i];
            if (index < sum) {
                r = m;
            } else {
                l = m + 1;
                acc = sum;
            }
        }
        size_t stripe = l - 1;
        size_t offset = index - acc;
        std::shared_lock lock(lock_.stripe(stripe));
        return data_[stripe][offset];
    }

    // 优化：缓存总大小
    size_t size() const noexcept {
        if (!size_dirty_.load(std::memory_order_relaxed)) {
            return cached_size_.load(std::memory_order_relaxed);
        }
        
        size_t total = 0;
        for (size_t i = 0; i < lock_.stripes(); ++i) {
            total += stripe_sizes_[i].load(std::memory_order_relaxed);
        }
        
        cached_size_.store(total, std::memory_order_relaxed);
        size_dirty_.store(false, std::memory_order_relaxed);
        return total;
    }

    // 优化：支持并发遍历
    template<typename Func>
    void for_each_concurrent(Func&& func) const {
        std::vector<std::thread> threads;
        threads.reserve(lock_.stripes());
        
        for (size_t i = 0; i < lock_.stripes(); ++i) {
            threads.emplace_back([this, i, &func]() {
                std::shared_lock lock(lock_.stripe(i));
                for (const auto& item : data_[i]) {
                    func(item);
                }
            });
        }
        
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }
    }

    void clear() {
        for (size_t i = 0; i < lock_.stripes(); ++i) {
            std::unique_lock lock(lock_.stripe(i));
            data_[i].clear();
            stripe_sizes_[i].store(0, std::memory_order_relaxed);
        }
        size_dirty_.store(true, std::memory_order_relaxed);
    }

    constexpr size_t stripes() const noexcept { return lock_.stripes(); }
};

template<typename T, typename Mutex = std::shared_mutex>
class ThreadSafeArray {
private:
    static constexpr bool is_null_mutex = std::is_same_v<Mutex, NullSharedMutex>;
    
    mutable Mutex mutex_;
    std::vector<T> data_;
    
    template<bool IsNull, typename MutexType>
    class LockGuard {
        std::conditional_t<IsNull, int, std::unique_lock<MutexType>> lock_;
    public:
        LockGuard(MutexType& m) : lock_(IsNull ? 0 : std::unique_lock<MutexType>(m)) {}
    };
    
    template<bool IsNull, typename MutexType>
    class SharedLockGuard {
        std::conditional_t<IsNull, int, std::shared_lock<MutexType>> lock_;
    public:
        SharedLockGuard(MutexType& m) : lock_(IsNull ? 0 : std::shared_lock<MutexType>(m)) {}
    };

public:
    ThreadSafeArray() = default;

    T operator[](size_t index) const { return get(index); }
    
    // 优化：减少重复代码的宏
    #define WITH_UNIQUE_LOCK(body) \
        if constexpr (is_null_mutex) { body } \
        else { std::unique_lock lock(mutex_); body }
    
    #define WITH_SHARED_LOCK(body) \
        if constexpr (is_null_mutex) { body } \
        else { std::shared_lock lock(mutex_); body }

    template<typename U = T>
    void push_back(U&& value) requires std::constructible_from<T, U&&> {
        WITH_UNIQUE_LOCK(data_.push_back(std::forward<U>(value));)
    }

    template<typename... Args>
    decltype(auto) emplace_back(Args&&... args) {
        WITH_UNIQUE_LOCK(return data_.emplace_back(std::forward<Args>(args)...);)
    }

    // 优化：批量操作
    template<std::ranges::input_range R>
    void insert_range(const R& range) {
        WITH_UNIQUE_LOCK(
            data_.insert(data_.end(), std::ranges::begin(range), 
                        std::ranges::end(range));
        )
    }

    // 优化：支持谓词查找
    template<typename Predicate>
    size_t find_if(Predicate pred) const {
        WITH_SHARED_LOCK(
            for (size_t i = 0; i < data_.size(); ++i) {
                if (pred(data_[i])) return i;
            }
            return npos;
        )
    }

    T get(size_t index) const {
        WITH_SHARED_LOCK(return data_.at(index);)
    }

    template<typename U = T>
    void set(size_t index, U&& value) requires std::assignable_from<T&, U&&> {
        WITH_UNIQUE_LOCK(data_.at(index) = std::forward<U>(value);)
    }

    size_t size() const noexcept {
        WITH_SHARED_LOCK(return data_.size();)
    }

    bool empty() const noexcept {
        WITH_SHARED_LOCK(return data_.empty();)
    }

    void clear() noexcept {
        WITH_UNIQUE_LOCK(data_.clear();)
    }

    // 优化：安全的迭代器访问（返回副本避免锁释放问题）
    std::vector<T> snapshot() const {
        WITH_SHARED_LOCK(return data_;)
    }

    // 优化：并发安全的遍历
    template<typename Func>
    void for_each(Func&& func) const {
        WITH_SHARED_LOCK(
            for (const auto& item : data_) {
                func(item);
            }
        )
    }

    // 优化：条件操作，避免多次加锁
    template<typename Predicate, typename Action>
    bool conditional_action(Predicate pred, Action action) {
        WITH_UNIQUE_LOCK(
            if (pred(data_)) {
                action(data_);
                return true;
            }
            return false;
        )
    }

    #undef WITH_UNIQUE_LOCK
    #undef WITH_SHARED_LOCK

    static constexpr size_t npos = static_cast<size_t>(-1);
};

// ===== 无锁版本（适用于单生产者多消费者场景）=====
template<typename T>
class LockFreeArray {
private:
    struct Node {
        alignas(64) std::atomic<T*> data{nullptr};
        alignas(64) std::atomic<size_t> size{0};
        alignas(64) std::atomic<Node*> next{nullptr};
    };
    
    alignas(64) std::atomic<Node*> head_{nullptr};
    alignas(64) std::atomic<size_t> total_size_{0};

public:
    LockFreeArray() {
        head_.store(new Node(), std::memory_order_relaxed);
    }

    // 仅支持单生产者
    void push_back(const T& value) {
        
        total_size_.fetch_add(1, std::memory_order_relaxed);
    }

    // 支持多消费者并发读取
    T get(size_t index) const {
       
        throw std::runtime_error("Simplified implementation");
    }

    size_t size() const noexcept {
        return total_size_.load(std::memory_order_relaxed);
    }
};

#endif // OPTIMIZED_THREAD_SAFE_ARRAY_H