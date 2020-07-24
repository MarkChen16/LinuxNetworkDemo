#pragma once

#include "ticketWindow.h"

//自旋锁：
//适合工作线程不多（内核数量-1)*2，实时性要求很高，临界时间很短的场景，否则会白白浪费很多CPU时间；
class SpinLocker
{
public:
	explicit SpinLocker(pthread_spinlock_t* spin);
	virtual ~SpinLocker();

protected:
	pthread_spinlock_t* m_spin;

};

class SpinTicketDB : public TicketDB
{
public:
	static SpinTicketDB* getInstance();

	virtual ~SpinTicketDB();

	void addTicket(int ticketNum) override;
	void removeTicket(int ticketNum) override;
	bool buyTicket(int windowID, int buyNum) override;
	int queryTicket() override;

private:
	explicit SpinTicketDB();

	pthread_spinlock_t m_spin;
};

#define STDB SpinTicketDB::getInstance()
