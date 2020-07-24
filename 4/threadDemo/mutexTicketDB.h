#pragma once

#include "ticketWindow.h"

//互斥锁：临界使用，适应于简单的场景
class MutexLocker
{
public:
	explicit MutexLocker(pthread_mutex_t* mutex);
	virtual ~MutexLocker();

protected:
	pthread_mutex_t* m_mutex;
};

//火车票数据库
class MutexTicketDB : public TicketDB
{
public:
	static MutexTicketDB* getInstance();

	virtual ~MutexTicketDB();

	void addTicket(int ticketNum) override;
	void removeTicket(int ticketNum) override;
	bool buyTicket(int windowID, int buyNum) override;
	int queryTicket() override;

private:
	explicit MutexTicketDB();

	pthread_mutex_t m_mutex;
};

#define MTDB MutexTicketDB::getInstance()
