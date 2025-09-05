#include <atomic>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <barrier>
#include "../include/thread_safe_array.h"
#include "../../lock/thread_safe_lock.h"
using namespace std::chrono;
constexpr size_t N = 1'000'000;
constexpr size_t THREADS = 8;
// 混合读写测试：读写比例可调，模拟真实业务
template<typename ArrayType>
void mixed_read_write_test(ArrayType& arr, size_t n, size_t threads, size_t read_ratio) {
	std::barrier sync(threads);
	std::atomic<size_t> write_idx{0};
	auto worker = [&](size_t tid) {
		std::mt19937_64 rng(tid);
		sync.arrive_and_wait();
		for (size_t i = 0; i < n / threads; ++i) {
			if (i % (read_ratio + 1) == 0) {
				arr.push_back(i + tid * n);
				write_idx.fetch_add(1, std::memory_order_relaxed);
			} else {
				size_t idx = rng() % (write_idx.load(std::memory_order_relaxed) + 1);
				if (idx < arr.size()) {
					auto v = arr[idx];
					v--;
				}
			}
		}
	};
	std::vector<std::thread> ts;
	for (size_t t = 0; t < threads; ++t) ts.emplace_back(worker, t);
	for (auto& th : ts) th.join();
}

// 批量写入测试
template<typename ArrayType>
void batch_push_back_test(ArrayType& arr, size_t n, size_t threads, size_t batch_size) {
	std::barrier sync(threads);
	auto worker = [&](size_t tid) {
		sync.arrive_and_wait();
		std::vector<size_t> batch(batch_size);
		for (size_t i = tid * n / threads; i < (tid + 1) * n / threads; i += batch_size) {
			for (size_t j = 0; j < batch_size && i + j < (tid + 1) * n / threads; ++j)
				batch[j] = i + j;
			arr.insert_range(batch);
		}
	};
	std::vector<std::thread> ts;
	for (size_t t = 0; t < threads; ++t) ts.emplace_back(worker, t);
	for (auto& th : ts) th.join();
}

// 随机访问测试
template<typename ArrayType>
void random_access_test(ArrayType& arr, size_t n, size_t threads) {
	std::barrier sync(threads);
	auto worker = [&](size_t tid) {
		std::mt19937_64 rng(tid);
		sync.arrive_and_wait();
		size_t sum = 0;
		for (size_t i = 0; i < n / threads; ++i) {
			size_t idx = rng() % arr.size();
			sum += arr[idx];
		}
	};
	std::vector<std::thread> ts;
	for (size_t t = 0; t < threads; ++t) ts.emplace_back(worker, t);
	for (auto& th : ts) th.join();
}


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

	constexpr size_t READ_RATIO = 10; // 读写比例10:1
	constexpr size_t BATCH_SIZE = 128;

	StripedThreadSafeArray<size_t> arr4;
	auto t1s = high_resolution_clock::now();
	test_push_back(arr4, N, THREADS);
	auto t2s = high_resolution_clock::now();
	test_read(arr4, N, THREADS);
	auto t3s = high_resolution_clock::now();
	std::cout << "StripedThreadSafeArray push_back:   " << duration_cast<milliseconds>(t2s-t1s).count() << " ms\n";
	std::cout << "StripedThreadSafeArray read:        " << duration_cast<milliseconds>(t3s-t2s).count() << " ms\n";

	// 混合读写
	arr4.clear();
	auto t4s = high_resolution_clock::now();
	mixed_read_write_test(arr4, N, THREADS, READ_RATIO);
	auto t5s = high_resolution_clock::now();
	std::cout << "StripedThreadSafeArray mixed read/write: " << duration_cast<milliseconds>(t5s-t4s).count() << " ms\n";

	// 批量写入
	arr4.clear();
	auto t6s = high_resolution_clock::now();
	batch_push_back_test(arr4, N, THREADS, BATCH_SIZE);
	auto t7s = high_resolution_clock::now();
	std::cout << "StripedThreadSafeArray batch push_back:  " << duration_cast<milliseconds>(t7s-t6s).count() << " ms\n";

	// 随机访问
	auto t8s = high_resolution_clock::now();
	random_access_test(arr4, N, THREADS);
	auto t9s = high_resolution_clock::now();
	std::cout << "StripedThreadSafeArray random access:    " << duration_cast<milliseconds>(t9s-t8s).count() << " ms\n";

	// 0b. std::vector 多线程push_back（加全局锁保证安全）
	std::vector<size_t> stdvec_mt;
	std::mutex stdvec_mutex;
	auto t0mt = high_resolution_clock::now();
	std::barrier sync_mt(THREADS);
	auto worker_mt = [&](size_t tid) {
		sync_mt.arrive_and_wait();
		for (size_t i = tid * N / THREADS; i < (tid + 1) * N / THREADS; ++i) {
			std::lock_guard<std::mutex> lock(stdvec_mutex);
			stdvec_mt.push_back(i);
		}
	};
	std::vector<std::thread> ts_mt;
	for (size_t t = 0; t < THREADS; ++t) ts_mt.emplace_back(worker_mt, t);
	for (auto& th : ts_mt) th.join();
	auto t1mt = high_resolution_clock::now();
	size_t sum_mt = 0;
	for (size_t i = 0; i < stdvec_mt.size(); ++i) sum_mt += stdvec_mt[i];
	auto t2mt = high_resolution_clock::now();
	std::cout << "std::vector (multi-thread, global lock) push_back: " << duration_cast<milliseconds>(t1mt-t0mt).count() << " ms\n";
	std::cout << "std::vector (multi-thread) read:      " << duration_cast<milliseconds>(t2mt-t1mt).count() << " ms\n";

	// std::vector 混合读写
	stdvec_mt.clear();
	auto t4mt = high_resolution_clock::now();
	mixed_read_write_test(stdvec_mt, N, THREADS, READ_RATIO);
	auto t5mt = high_resolution_clock::now();
	std::cout << "std::vector (multi-thread, global lock) mixed read/write: " << duration_cast<milliseconds>(t5mt-t4mt).count() << " ms\n";

	// 0. std::vector 单线程性能对比
	std::vector<size_t> stdvec;
	auto t0 = high_resolution_clock::now();

	// 1. std::shared_mutex 混合读写

	arr1.clear();
	auto t4 = high_resolution_clock::now();
	mixed_read_write_test(arr1, N, THREADS, READ_RATIO);
	auto t5 = high_resolution_clock::now();
	std::cout << "std::shared_mutex mixed read/write: " << duration_cast<milliseconds>(t5-t4).count() << " ms\n";

	// 2. SpinSharedMutex 混合读写

	arr2.clear();
	t4 = high_resolution_clock::now();
	mixed_read_write_test(arr2, N, THREADS, READ_RATIO);
	t5 = high_resolution_clock::now();
	std::cout << "SpinSharedMutex mixed read/write:   " << duration_cast<milliseconds>(t5-t4).count() << " ms\n";

	// 3. NullSharedMutex 混合读写（只允许单线程写）

	arr3.clear();

	t4 = high_resolution_clock::now();
	mixed_read_write_test(arr3, N, 1, READ_RATIO);
	t5 = high_resolution_clock::now();
	std::cout << "NullSharedMutex mixed read/write:   " << duration_cast<milliseconds>(t5-t4).count() << " ms\n";
	for (size_t i = 0; i < N; ++i) stdvec.push_back(i);
	auto t1v = high_resolution_clock::now();
	size_t sum = 0;
	for (size_t i = 0; i < N; ++i) sum += stdvec[i];
	auto t2v = high_resolution_clock::now();
	std::cout << "std::vector push_back:       " << duration_cast<milliseconds>(t1v-t0).count() << " ms\n";
	std::cout << "std::vector read:            " << duration_cast<milliseconds>(t2v-t1v).count() << " ms\n";
	using namespace std::chrono;
	std::cout << "==== ThreadSafeArray 性能测试 ====" << std::endl;


	return 0;
}
