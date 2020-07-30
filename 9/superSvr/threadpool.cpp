#include "threadpool.h"
#include <assert.h>
#include <unistd.h>

ThreadPool* ThreadPool::gs_pool = NULL;

ThreadPool* ThreadPool::getInstance()
{
	if (!gs_pool)
	{
		gs_pool = new ThreadPool();
		atexit(ThreadPool::atexit_handler);
	}

	return gs_pool;
}

ThreadPool::ThreadPool(int activeThreadCount)
	: m_shutdown(false)
{
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init(&m_cond, NULL);

	//查询核心数
	if (activeThreadCount <= 0)
	{
		activeThreadCount = sysconf(_SC_NPROCESSORS_ONLN);
	}

	//至少开一个线程
	if (activeThreadCount <= 0)
	{
		activeThreadCount = 1;
	}

	//开启工作线程
	int result = 0;
	for (int i = 0; i < activeThreadCount; i++)
	{
		pthread_t* thr = new pthread_t();
		m_thrList.push_back(thr);

		result = pthread_create(thr, NULL, ThreadPool::run, this);
		if (result == -1)
		{
			perror("ThreadPool()::pthread_create");
		}
	}
}

ThreadPool::~ThreadPool()
{
	shutdown();
	clearTask();

	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_cond);
}

void ThreadPool::addTask(const std::function<void(void*)>& fun, void* arg, bool priority)
{
	//添加任务，唤醒工作线程
	MutexLocker locker(&m_mutex);
	
	__task_t* newTask = new __task_t();
	newTask->fun = fun;
	newTask->arg = arg;

	if (priority)
		m_queue.push_front(newTask);
	else
		m_queue.push_back(newTask);

	pthread_cond_signal(&m_cond);
}

void ThreadPool::clearTask()
{
	//清除任务
	MutexLocker locker(&m_mutex);

	while (!m_queue.empty())
	{
		delete m_queue.back();
		m_queue.pop_back();
	}
}

void ThreadPool::shutdown()
{
	if (m_shutdown)
		return;

	//请求关闭，唤醒工作线程
	{
		MutexLocker locker(&m_mutex);

		m_shutdown = true;

		pthread_cond_broadcast(&m_cond);
	}

	//等待工作线程关闭并删除
	for (int i = 0; i < m_thrList.size(); i++)
	{
		pthread_join(*m_thrList[i], NULL);

		delete m_thrList[i];
		m_thrList[i] = NULL;
	}
}

ThreadPool::__task_t* ThreadPool::pickTask()
{
	//选取任务，等待任务或者请求关闭
	MutexLocker locker(&m_mutex);

	while (m_queue.empty() && !m_shutdown)
	{
		pthread_cond_wait(&m_cond, &m_mutex);
	}
	
	__task_t* frontTask = NULL;
	if (!m_queue.empty() && !m_shutdown)
	{
		frontTask = m_queue.front();
		m_queue.pop_front();
	}

	return frontTask;
}

void* ThreadPool::run(void* arg)
{
	ThreadPool* This = (ThreadPool*)arg;
	assert(This);

	while (true)
	{
		//选取任务
		__task_t* task = This->pickTask();
		if (!task)
			break;

		//执行任务
		try
		{
			task->fun(task->arg);
		}
		catch (...)
		{
			printf("Task::exec() catch an exception.\n");
		}

		//删除任务
		delete task;
		task = NULL;
	}

	pthread_exit(NULL);
}

void ThreadPool::atexit_handler()
{
	//这里只会关闭全局线程池，私有线程池要自行关闭
	if (gs_pool)
	{
		delete gs_pool;
		gs_pool = NULL;
	}
}

MutexLocker::MutexLocker(pthread_mutex_t* mutex)
	: m_mutex(mutex)
{
	assert(m_mutex);

	pthread_mutex_lock(m_mutex);
}

MutexLocker::~MutexLocker()
{
	pthread_mutex_unlock(m_mutex);
}
