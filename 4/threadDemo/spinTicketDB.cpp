#include "spinTicketDB.h"

SpinLocker::SpinLocker(pthread_spinlock_t* spin)
	: m_spin(spin)
{
	pthread_spin_lock(m_spin);
}

SpinLocker::~SpinLocker()
{
	pthread_spin_unlock(m_spin);
}

SpinTicketDB* SpinTicketDB::getInstance()
{
	static SpinTicketDB db;

	return &db;
}

SpinTicketDB::SpinTicketDB()
	: TicketDB()
{
	//初始化互斥量
	pthread_spin_init(&m_spin, PTHREAD_PROCESS_SHARED);
}

SpinTicketDB::~SpinTicketDB()
{
	//释放互斥量
	pthread_spin_destroy(&m_spin);
}

void SpinTicketDB::addTicket(int ticketNum)
{
	SpinLocker locker(&m_spin);

	m_ticketCount += ticketNum;
	printf("数据库增加火车票%d张，余量%d张！\n", ticketNum, m_ticketCount);

	TicketDB::addTicket(ticketNum);
}

void SpinTicketDB::removeTicket(int ticketNum)
{
	SpinLocker locker(&m_spin);

	m_ticketCount -= ticketNum;
	printf("数据库减少火车票%d张，余量%d张！\n", ticketNum, m_ticketCount);
}

bool SpinTicketDB::buyTicket(int windowID, int buyNum)
{
	bool isSuccess = false;
	int currTicketCount = 0;

	//自旋锁适合要求实时性很高，临界时间很短的场景，否则会白白浪费很多CPU时间；
	{
		SpinLocker locker(&m_spin);

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

int SpinTicketDB::queryTicket()
{
	SpinLocker locker(&m_spin);

	return m_ticketCount;
}
