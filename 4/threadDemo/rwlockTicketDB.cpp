#include "rwlockTicketDB.h"

RWLocker::RWLocker(pthread_rwlock_t* rwlock, bool isRead)
	: m_pRWlock(rwlock)
{
	if (isRead)
		pthread_rwlock_rdlock(m_pRWlock);
	else
		pthread_rwlock_wrlock(m_pRWlock);
}

RWLocker::~RWLocker()
{
	pthread_rwlock_unlock(m_pRWlock);
}

RWLockTicketDB* RWLockTicketDB::getInstance()
{
	static RWLockTicketDB db;

	return &db;
}

RWLockTicketDB::~RWLockTicketDB()
{
	pthread_rwlock_destroy(&m_rwlock);
}

void RWLockTicketDB::addTicket(int ticketNum)
{
	RWLocker locker(&m_rwlock, false);

	m_ticketCount += ticketNum;
	printf("数据库增加火车票%d张，余量%d张！\n", ticketNum, m_ticketCount);

	TicketDB::addTicket(ticketNum);
}

void RWLockTicketDB::removeTicket(int ticketNum)
{
	RWLocker locker(&m_rwlock, false);

	m_ticketCount -= ticketNum;
	printf("数据库减少火车票%d张，余量%d张！\n", ticketNum, m_ticketCount);
}

bool RWLockTicketDB::buyTicket(int windowID, int buyNum)
{
	bool isSuccess = false;
	int currTicketCount = 0;

	//售票窗口允许重新发起请求，原子变量不空旋转
	{
		RWLocker locker(&m_rwlock, false);

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

int RWLockTicketDB::queryTicket()
{
	RWLocker locker(&m_rwlock, true);

	return m_ticketCount;
}

RWLockTicketDB::RWLockTicketDB()
	: TicketDB()
{
	//避免写线程饥饿
	pthread_rwlockattr_t attr;
	pthread_rwlockattr_init(&attr);
	pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);

	pthread_rwlock_init(&m_rwlock, &attr);
}
