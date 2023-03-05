#include "ThreadPool.h"

#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

static std::atomic<int> taskCounter{0}; //原子变量不能使用拷贝构造
// std::mutex processMutex;

static void process()
{
	for (volatile auto index = 0UL; index < 10000UL; ++index)
		;
	std::this_thread::sleep_for(std::chrono::milliseconds(3));
	taskCounter.fetch_add(1, std::memory_order_relaxed);
}

void eterfree(ThreadPool &threadPool)
{

	for (int i = 0; i < 300000; ++i)
	{ //任务数量
		threadPool.enqueue(process);
	}
}

int main()
{
	ThreadPool threadPool(100); //创建线程

	auto start = std::chrono::steady_clock::now();

	eterfree(threadPool);

	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end -
																		  start);

	std::this_thread::sleep_for(std::chrono::milliseconds(10000));
	std::cout << "任务数量：" << taskCounter << " 个" << std::endl;
	std::cout << "执行时间：" << duration.count() << " ms" << std::endl;

	return 0;
}