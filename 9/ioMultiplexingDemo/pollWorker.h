#pragma once

#include "baseWorker.h"

#include <pthread.h>
#include <atomic>
#include <vector>
#include <map>


/*
poll IO复用：
适用于描述符量级中等，采用轮询socket状态的方式；

相关函数：poll

*/

class PollWorker : public baseWorker
{
public:
	static PollWorker* getInstance();

	explicit PollWorker();
	virtual ~PollWorker();

	bool start() override;
	void askForExit() override;
	void stop() override;

	void addClient(clientInfo* info) override;

protected:
	static void* run(void* arg);

private:
	pthread_t m_thr;
	std::atomic_bool m_askForExit;

	pthread_spinlock_t m_spin;
	std::vector<clientInfo*> m_addClientList;
	bool m_hasNewClient;
};

#define PLWORKER PollWorker::getInstance()
