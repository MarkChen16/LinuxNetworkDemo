#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>

//原子变量
#include <atomic>


class TicketDB;
class TicketWindow;

class TicketDB
{
public:
	explicit TicketDB()
		: m_ticketCount(0)
	{
		pthread_cond_init(&m_cond, NULL);
		pthread_mutex_init(&m_mutexWait, NULL);
		pthread_mutex_init(&m_mutexSignal, NULL);
	}

	virtual ~TicketDB()
	{
		pthread_cond_destroy(&m_cond);
		pthread_mutex_destroy(&m_mutexWait);
		pthread_mutex_destroy(&m_mutexSignal);
	}

	virtual void addTicket(int ticketNum);
	virtual void removeTicket(int ticketNum) = 0;
	virtual bool buyTicket(int windowID, int buyNum) = 0;
	virtual int queryTicket() = 0;

	void waitPutTicket();

protected:
	int m_ticketCount;

	pthread_cond_t m_cond;
	pthread_mutex_t m_mutexWait;
	pthread_mutex_t m_mutexSignal;
};

//售票窗口
class TicketWindow
{
public:
	explicit TicketWindow(TicketDB* TDB, int windowID = 0);
	virtual ~TicketWindow();

	bool start();
	void askForExit();
	void stop();

	int getWindowID() const;

	void waitPutTicket();
	void buyTicket(int buyCount);

protected:
	static void* run(void *arg);

private:
	TicketDB* m_TDB;
	int m_windowID;

	//使用原子变量替换互斥锁
	std::atomic_bool m_askForExit;
	pthread_t m_thr;
};
