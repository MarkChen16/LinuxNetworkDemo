#include "atomicTicketDB.h"

AtomicLocker::AtomicLocker(std::atomic_bool* atomic)
	: m_pAtomic(atomic)
	, m_now(false)
	, m_val(true)
	, m_lockUp(false)
{
}

AtomicLocker::~AtomicLocker()
{
	//如果已经上锁，还原锁状态
	if (m_lockUp)
	{
		m_now = !m_now;
		m_val = !m_val;

		//原子变量空旋转
		while (m_pAtomic->compare_exchange_strong(m_now, m_val, std::memory_order_seq_cst) == false) {};
	}
}

bool AtomicLocker::tryLock(bool now, bool val, bool needSpin)
{
	m_now = now;
	m_val = val;

	//读-修改-写原子操作
	if (needSpin)
	{
		//失败空旋转
		while (m_pAtomic->compare_exchange_strong(m_now, m_val, std::memory_order_seq_cst) == false) {};

		m_lockUp = true;
	}
	else
	{
		//失败直接返回
		m_lockUp = m_pAtomic->compare_exchange_strong(m_now, m_val, std::memory_order_seq_cst);
	}

	return m_lockUp;
}

AtomicTicketDB* AtomicTicketDB::getInstance()
{
	static AtomicTicketDB db;

	return &db;
}

AtomicTicketDB::~AtomicTicketDB()
{
}

void AtomicTicketDB::addTicket(int ticketNum)
{
	AtomicLocker locker(&m_atomic);
	locker.tryLock(false, true, true);

	m_ticketCount += ticketNum;
	printf("数据库增加火车票%d张，余量%d张！\n", ticketNum, m_ticketCount);

	TicketDB::addTicket(ticketNum);
}

void AtomicTicketDB::removeTicket(int ticketNum)
{
	AtomicLocker locker(&m_atomic);
	locker.tryLock(false, true, true);

	m_ticketCount -= ticketNum;
	printf("数据库减少火车票%d张，余量%d张！\n", ticketNum, m_ticketCount);
}

bool AtomicTicketDB::buyTicket(int windowID, int buyNum)
{
	bool isSuccess = false;
	int currTicketCount = 0;

	//售票窗口允许重新发起请求，原子变量不空旋转
	{
		AtomicLocker locker(&m_atomic);
		if (locker.tryLock(false, true, false) == false)
			return false;

		if (buyNum <= m_ticketCount)
		{
			isSuccess = true;
			m_ticketCount -= buyNum;
		}

		currTicketCount = m_ticketCount;
	}

	if (isSuccess)
	{
		printf("窗口%05d出售%d张火车票成功，余量还有%d张！\n", windowID, buyNum, currTicketCount);
	}
	else
	{
		printf("窗口%05d出售%d张火车票失败，余量不足只有%d张！\n", windowID, buyNum, currTicketCount);
	}
}

int AtomicTicketDB::queryTicket()
{
	AtomicLocker locker(&m_atomic);
	if (locker.tryLock(false, true, false))
		return m_ticketCount;
	else
		return -1;
}

AtomicTicketDB::AtomicTicketDB()
	: TicketDB()
	, m_atomic(false)
{
}
