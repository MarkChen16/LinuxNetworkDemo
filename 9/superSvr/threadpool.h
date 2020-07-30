#pragma once

#include <vector>
#include <queue>
#include <deque>
#include <list>
#include <map>
#include <functional>
#include <atomic>

using namespace std;

/*
线程池：若干工作线程的集合，一般为内核数量，有任务就执行，没有任务休眠；
*/

class MutexLocker;
class ThreadPool;

class ThreadPool
{
private:
	struct __task_t
	{
		function<void(void*)> fun;
		void* arg;
	};

public:
	static ThreadPool* getInstance();

	explicit ThreadPool(int activeThreadCount = 0);
	virtual ~ThreadPool();

	void addTask(const std::function<void(void*)>& fun, void* arg = NULL, bool priority = false);
	void clearTask();
	void shutdown();

protected:
	__task_t* pickTask();

private:
	static void* run(void *arg);
	static void atexit_handler();

private:
	vector<pthread_t*> m_thrList;
	std::atomic_bool m_shutdown;

	pthread_mutex_t m_mutex;
	pthread_cond_t m_cond;
	std::list<__task_t*> m_queue;

	static ThreadPool* gs_pool;
};

class MutexLocker
{
public:
	explicit MutexLocker(pthread_mutex_t* mutex);
	virtual ~MutexLocker();

protected:
	pthread_mutex_t* m_mutex;
};
