#pragma once

#include "baseWorker.h"

#include <pthread.h>
#include <atomic>
#include <vector>
#include <map>


/*
select IO复用：
适用于描述符量级小，默认是1024，采用轮询socket状态的方式；

相关函数：select

*/

class SelectWorker : public baseWorker
{
public:
	static SelectWorker* getInstance();

	explicit SelectWorker();
	virtual ~SelectWorker();

	bool start() override;
	void askForExit() override;
	void stop() override;

	void addClient(clientInfo* info) override;

protected:
	static void* run(void* arg);

private:
	static int getMaxFD(std::vector<clientInfo*>& cinfoList);

private:
	pthread_t m_thr;
	std::atomic_bool m_askForExit;

	pthread_spinlock_t m_spin;
	std::vector<clientInfo*> m_addClientList;
	bool m_hasNewClient;
};

#define SLWORKER SelectWorker::getInstance()
