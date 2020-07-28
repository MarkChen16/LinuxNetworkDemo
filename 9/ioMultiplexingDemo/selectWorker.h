#pragma once

#include "baseWorker.h"

#include <pthread.h>
#include <atomic>
#include <vector>
#include <map>


/*
select IO���ã�
����������������С��Ĭ����1024��������ѯsocket״̬�ķ�ʽ��

��غ�����select

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
