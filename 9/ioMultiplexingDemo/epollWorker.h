#pragma once

#include "baseWorker.h"

#include <pthread.h>
#include <atomic>
#include <assert.h>
#include <vector>

#define EPOLL_SIZE 20000
#define WAIT_COUNT 1000
#define BUFF_LEN 1024

/*
epoll IO复用（高并发服务端）：
适用于描述符量级比较大的场景，通过注册fd和回调函数，内部使用红黑树查询；

LT模式：标准模式(默认)，适用于阻塞和非阻塞socket；如果通知后，不做处理，或者没有处理完，下一次还会通知；
ET模式：高效模式，适用于非阻塞socket；同一个状态在同一时间段内只通知一次；

epoll相关函数： epoll_create  epoll_ctl  epoll_wait

*/

class EpollWorker : public baseWorker
{
public:
	static EpollWorker* getInstance();

	explicit EpollWorker();
	virtual ~EpollWorker();

	bool start() override;
	void askForExit() override;
	void stop() override;

	void addClient(clientInfo* info) override;

protected:
	static void* run(void *arg);

private:
	pthread_t m_thr;
	std::atomic_bool m_askForExit;

	pthread_spinlock_t m_spin;
	std::vector<clientInfo*> m_addClientList;
	bool m_hasNewClient;
};

#define EPWORKER EpollWorker::getInstance()
