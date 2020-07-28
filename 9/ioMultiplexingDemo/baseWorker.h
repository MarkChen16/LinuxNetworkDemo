#pragma once

#include <pthread.h>

#define BUFF_LEN 1024

struct clientInfo
{
	int fd;
	char addr[100];
	int port;
};

class baseWorker
{
public:
	virtual bool start() = 0;
	virtual void askForExit() = 0;
	virtual void stop() = 0;

	virtual void addClient(clientInfo* info) = 0;
};

class SpinLocker
{
public:
	explicit SpinLocker(pthread_spinlock_t* spin);
	virtual ~SpinLocker();

protected:
	pthread_spinlock_t* m_spin;

};
