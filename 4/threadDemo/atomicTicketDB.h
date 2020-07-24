#pragma once

#include <atomic>
#include "ticketWindow.h"

//原子变量：最原始的锁，也是互斥锁的原理
class AtomicLocker
{
public:
	explicit AtomicLocker(std::atomic_bool* atomic);
	virtual ~AtomicLocker();

	bool tryLock(bool now, bool val, bool needSpin);

private:
	std::atomic_bool* m_pAtomic;

	bool m_now;
	bool m_val;
	bool m_lockUp;
};

class AtomicTicketDB : public TicketDB
{
public:
	static AtomicTicketDB* getInstance();

	virtual ~AtomicTicketDB();

	void addTicket(int ticketNum) override;
	void removeTicket(int ticketNum) override;
	bool buyTicket(int windowID, int buyNum) override;
	int queryTicket() override;

private:
	explicit AtomicTicketDB();

	std::atomic_bool m_atomic;
};

#define ATDB AtomicTicketDB::getInstance()
