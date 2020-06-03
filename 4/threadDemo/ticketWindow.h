#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>

class MutexLocker;
class TicketDB;
class TicketWindow;

//互斥锁
class MutexLocker
{
public:
	explicit MutexLocker(pthread_mutex_t* mutex);
	virtual ~MutexLocker();

protected:
	pthread_mutex_t* m_mutex;
};

//火车票数据库
class TicketDB
{
public:
	static TicketDB& getInstance();

	virtual ~TicketDB();

	void addTicket(int ticketNum);
	void removeTicket(int ticketNum);
	bool buyTicket(const TicketWindow& window, int buyNum);

protected:


private:
	explicit TicketDB();

private:
	int m_currTicketNo;
	int m_ticketCount;

	pthread_mutex_t m_mutex;
};

#define TDB TicketDB::getInstance()

//售票窗口
class TicketWindow
{
public:
	explicit TicketWindow(int windowID = 0);
	virtual ~TicketWindow();

	bool start();
	void askForExit();
	void stop();

	int getWindowID() const;
	void setWindowID(int id);

protected:
	static void* run(void *arg);

private:
	int m_windowID;

	bool m_askForExit;
	pthread_mutex_t m_mutex;

	pthread_t m_thr;

};
