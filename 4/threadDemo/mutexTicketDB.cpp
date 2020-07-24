#include "mutexTicketDB.h"

MutexLocker::MutexLocker(pthread_mutex_t* mutex)
	: m_mutex(mutex)
{
	assert(m_mutex);

	pthread_mutex_lock(m_mutex);
}

MutexLocker::~MutexLocker()
{
	pthread_mutex_unlock(m_mutex);
}

MutexTicketDB* MutexTicketDB::getInstance()
{
	static MutexTicketDB db;

	return &db;
}

MutexTicketDB::MutexTicketDB()
	: TicketDB()
{
	//初始化互斥量
	pthread_mutex_init(&m_mutex, NULL);
}

MutexTicketDB::~MutexTicketDB()
{
	//释放互斥量
	pthread_mutex_destroy(&m_mutex);
}

void MutexTicketDB::addTicket(int ticketNum)
{
	MutexLocker locker(&m_mutex);

	m_ticketCount += ticketNum;
	printf("数据库增加火车票%d张，余量%d张！\n", ticketNum, m_ticketCount);

	TicketDB::addTicket(ticketNum);
}

void MutexTicketDB::removeTicket(int ticketNum)
{
	MutexLocker locker(&m_mutex);

	m_ticketCount -= ticketNum;
	printf("数据库减少火车票%d张，余量%d张！\n", ticketNum, m_ticketCount);
}

bool MutexTicketDB::buyTicket(int windowID, int buyNum)
{
	bool isSuccess = false;
	int currTicketCount = 0;

	//自旋锁适合要求实时性很高，临界时间很短的场景，否则会白白浪费很多CPU时间；
	{
		MutexLocker locker(&m_mutex);

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

int MutexTicketDB::queryTicket()
{
	MutexLocker locker(&m_mutex);

	return m_ticketCount;
}
