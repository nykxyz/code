public:
    // 批量插入，减少锁竞争
    template<std::ranges::input_range R>
    void insert_range(const R& range) {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.insert(data_.end(), std::ranges::begin(range), std::ranges::end(range));
        } else {
            UniqueLock lock(mutex_);
            data_.insert(data_.end(), std::ranges::begin(range), std::ranges::end(range));
        }
    }

    // 批量删除（移除区间）
    void erase_range(size_t first, size_t last) {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            if (first < last && last <= data_.size()) {
                data_.erase(data_.begin() + first, data_.begin() + last);
            }
        } else {
            UniqueLock lock(mutex_);
            if (first < last && last <= data_.size()) {
                data_.erase(data_.begin() + first, data_.begin() + last);
            }
        }
    }

    // 非阻塞插入，极致性能场景
    template<typename U = T>
    bool try_push_back(U&& value) requires std::constructible_from<T, U&&> {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.push_back(std::forward<U>(value));
            return true;
        } else {
            UniqueLock lock(mutex_, std::try_to_lock);
            if (lock.owns_lock()) {
                data_.push_back(std::forward<U>(value));
                return true;
            }
            return false;
        }
    }

    // 非阻塞原地构造
    template<typename... Args>
    bool try_emplace_back(Args&&... args) {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.emplace_back(std::forward<Args>(args)...);
            return true;
        } else {
            UniqueLock lock(mutex_, std::try_to_lock);
            if (lock.owns_lock()) {
                data_.emplace_back(std::forward<Args>(args)...);
                return true;
            }
            return false;
        }
    }

    // 仅在用户保证安全时暴露底层数据指针
    T* unsafe_data() noexcept { return data_.data(); }
    const T* unsafe_data() const noexcept { return data_.data(); }
#ifndef THREAD_SAFE_ARRAY_H
#define THREAD_SAFE_ARRAY_H

#include <vector>
#include <shared_mutex>
#include <type_traits>
#include "thread_safe_lock.h"
template<typename T, typename Mutex = std::shared_mutex>
class ThreadSafeArray {
    using UniqueLock = std::unique_lock<Mutex>;
    using SharedLock = std::shared_lock<Mutex>;
    public:

    template<std::ranges::input_range R>
    void insert_range(const R& range) {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.insert(data_.end(), std::ranges::begin(range), std::ranges::end(range));
        } else {
            UniqueLock lock(mutex_);
            data_.insert(data_.end(), std::ranges::begin(range), std::ranges::end(range));
        }
    }

    // 批量删除（移除区间）
    void erase_range(size_t first, size_t last) {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            if (first < last && last <= data_.size()) {
                data_.erase(data_.begin() + first, data_.begin() + last);
            }
        } else {
            UniqueLock lock(mutex_);
            if (first < last && last <= data_.size()) {
                data_.erase(data_.begin() + first, data_.begin() + last);
            }
        }
    }

    // 非阻塞插入，极致性能场景
    template<typename U = T>
    bool try_push_back(U&& value) requires std::constructible_from<T, U&&> {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.push_back(std::forward<U>(value));
            return true;
        } else {
            UniqueLock lock(mutex_, std::try_to_lock);
            if (lock.owns_lock()) {
                data_.push_back(std::forward<U>(value));
                return true;
            }
            return false;
        }
    }

    // 非阻塞原地构造
    template<typename... Args>
    bool try_emplace_back(Args&&... args) {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.emplace_back(std::forward<Args>(args)...);
            return true;
        } else {
            UniqueLock lock(mutex_, std::try_to_lock);
            if (lock.owns_lock()) {
                data_.emplace_back(std::forward<Args>(args)...);
                return true;
            }
            return false;
        }
    }

    T* unsafe_data() noexcept { return data_.data(); }
    const T* unsafe_data() const noexcept { return data_.data(); }
public:
    ThreadSafeArray() = default;
    // 支持左值和右值，完美转发
    template<typename U = T>
    void push_back(U&& value) requires std::constructible_from<T, U&&> {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.push_back(std::forward<U>(value));
        } else {
            UniqueLock lock(mutex_);
            data_.push_back(std::forward<U>(value));
        }
    }

    // 获取指定索引的元素，越界会抛出异常
    T get(size_t index) const {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            return data_.at(index);
        } else {
            SharedLock lock(mutex_);
            return data_.at(index);
        }
    }

    // 设置指定索引的元素值
    // 支持左值和右值，完美转发
    template<typename U = T>
    void set(size_t index, U&& value) requires std::assignable_from<T&, U&&> {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.at(index) = std::forward<U>(value);
        } else {
            UniqueLock lock(mutex_);
            data_.at(index) = std::forward<U>(value);
        }
    }
    // 原地构造元素
    template<typename... Args>
    decltype(auto) emplace_back(Args&&... args) {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            return data_.emplace_back(std::forward<Args>(args)...);
        } else {
            UniqueLock lock(mutex_);
            return data_.emplace_back(std::forward<Args>(args)...);
        }
    }
    // 预分配空间
    void reserve(size_t n) {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.reserve(n);
        } else {
            UniqueLock lock(mutex_);
            data_.reserve(n);
        }
    }

    // 获取容量
    size_t capacity() const {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            return data_.capacity();
        } else {
            SharedLock lock(mutex_);
            return data_.capacity();
        }
    }
    // C++23: 支持begin/end (只读)
    auto begin() const {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            return data_.begin();
        } else {
            SharedLock lock(mutex_);
            return data_.begin();
        }
    }
    auto end() const {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            return data_.end();
        } else {
            SharedLock lock(mutex_);
            return data_.end();
        }
    }

    // 支持下标运算符（不做越界检查）
    T operator[](size_t index) const {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            return data_[index];
        } else {
            SharedLock lock(mutex_);
            return data_[index];
        }
    }

    // 删除指定索引的元素
    void erase(size_t index) {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            if (index < data_.size()) {
                data_.erase(data_.begin() + index);
            }
        } else {
            UniqueLock lock(mutex_);
            if (index < data_.size()) {
                data_.erase(data_.begin() + index);
            }
        }
    }

    // 清空数组
    void clear() {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.clear();
        } else {
            UniqueLock lock(mutex_);
            data_.clear();
        }
    }

    // 查找元素，返回索引，未找到返回 npos
    size_t find(const T& value) const {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            for (size_t i = 0; i < data_.size(); ++i) {
                if (data_[i] == value) return i;
            }
            return npos;
        } else {
            SharedLock lock(mutex_);
            for (size_t i = 0; i < data_.size(); ++i) {
                if (data_[i] == value) return i;
            }
            return npos;
        }
    }

    // 判断数组是否为空
    bool empty() const {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            return data_.empty();
        } else {
            SharedLock lock(mutex_);
            return data_.empty();
        }
    }

    // 获取数组大小
    size_t size() const {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            return data_.size();
        } else {
            SharedLock lock(mutex_);
            return data_.size();
        }
    }

    // 交换内容
    void swap(ThreadSafeArray& other) {
        if (this == &other) return;
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            data_.swap(other.data_);
        } else {
            std::scoped_lock lock(mutex_, other.mutex_);
            data_.swap(other.data_);
        }
    }

    std::vector<T> to_vector() const {
        if constexpr (std::is_same_v<Mutex, NullSharedMutex>) {
            return data_;
        } else {
            SharedLock lock(mutex_);
            return data_;
        }
    }

    // npos 常量
    static constexpr size_t npos = static_cast<size_t>(-1);

private:
    mutable Mutex mutex_;
    std::vector<T> data_;
};

#endif // THREAD_SAFE_ARRAY_H

