#pragma once

#include "ticketWindow.h"

//读写锁：
//适合有读写操作业务，读操作比较多可以提高效率，读操作可以同时进行，不会造成数据混乱
class RWLocker
{
public:
	explicit RWLocker(pthread_rwlock_t* rwlock, bool isRead);
	virtual ~RWLocker();

private:
	pthread_rwlock_t* m_pRWlock;
};

class RWLockTicketDB : public TicketDB
{
public:
	static RWLockTicketDB* getInstance();

	virtual ~RWLockTicketDB();

	void addTicket(int ticketNum) override;
	void removeTicket(int ticketNum) override;
	bool buyTicket(int windowID, int buyNum) override;
	int queryTicket() override;

private:
	explicit RWLockTicketDB();

	pthread_rwlock_t m_rwlock;
};

#define RWTDB RWLockTicketDB::getInstance()


