#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <barrier>
#include "../include/thread_safe_array.h"
#include "../../lock/thread_safe_lock.h"

constexpr size_t N = 1'000'000;
constexpr size_t THREADS = 8;

template<typename ArrayType>
void test_push_back(ArrayType& arr, size_t n, size_t threads) {
	std::barrier sync(threads);
	auto worker = [&](size_t tid) {
		sync.arrive_and_wait();
		for (size_t i = tid * n / threads; i < (tid + 1) * n / threads; ++i) {
			arr.push_back(i);
		}
	};
	std::vector<std::thread> ts;
	for (size_t t = 0; t < threads; ++t) ts.emplace_back(worker, t);
	for (auto& th : ts) th.join();
}

template<typename ArrayType>
void test_read(ArrayType& arr, size_t n, size_t threads) {
	std::barrier sync(threads);
	auto worker = [&](size_t tid) {
		sync.arrive_and_wait();
		size_t sum = 0;
		for (size_t i = tid * n / threads; i < (tid + 1) * n / threads; ++i) {
			sum += arr[i];
		}
	};
	std::vector<std::thread> ts;
	for (size_t t = 0; t < threads; ++t) ts.emplace_back(worker, t);
	for (auto& th : ts) th.join();
}

int main() {
	using namespace std::chrono;
	std::cout << "==== ThreadSafeArray 性能测试 ====" << std::endl;

	// 1. std::shared_mutex 线程安全
	ThreadSafeArray<size_t, std::shared_mutex> arr1;
	auto t1 = high_resolution_clock::now();
	test_push_back(arr1, N, THREADS);
	auto t2 = high_resolution_clock::now();
	test_read(arr1, N, THREADS);
	auto t3 = high_resolution_clock::now();
	std::cout << "std::shared_mutex push_back: " << duration_cast<milliseconds>(t2-t1).count() << " ms\n";
	std::cout << "std::shared_mutex read:      " << duration_cast<milliseconds>(t3-t2).count() << " ms\n";

	// 2. SpinSharedMutex 线程安全
	ThreadSafeArray<size_t, SpinSharedMutex> arr2;
	t1 = high_resolution_clock::now();
	test_push_back(arr2, N, THREADS);
	t2 = high_resolution_clock::now();
	test_read(arr2, N, THREADS);
	t3 = high_resolution_clock::now();
	std::cout << "SpinSharedMutex push_back:   " << duration_cast<milliseconds>(t2-t1).count() << " ms\n";
	std::cout << "SpinSharedMutex read:        " << duration_cast<milliseconds>(t3-t2).count() << " ms\n";

	// 3. NullSharedMutex 非线程安全（只允许单线程写）
	ThreadSafeArray<size_t, NullSharedMutex> arr3;
	t1 = high_resolution_clock::now();
	test_push_back(arr3, N, 1); // 只用1个线程写，防止vector崩溃
	t2 = high_resolution_clock::now();
	test_read(arr3, N, THREADS); // 读可以多线程
	t3 = high_resolution_clock::now();
	std::cout << "NullSharedMutex push_back:   " << duration_cast<milliseconds>(t2-t1).count() << " ms\n";
	std::cout << "NullSharedMutex read:        " << duration_cast<milliseconds>(t3-t2).count() << " ms\n";

	return 0;
}
