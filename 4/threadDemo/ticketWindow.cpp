#include "ticketWindow.h"

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

TicketDB& TicketDB::getInstance()
{
	static TicketDB db;

	return db;
}

TicketDB::~TicketDB()
{
	//释放互斥量
	pthread_mutex_destroy(&m_mutex);
}

void TicketDB::addTicket(int ticketNum)
{
	MutexLocker locker(&m_mutex);

	m_ticketCount += ticketNum;
	printf("数据库增加火车票%d张，余量%d张！\n", ticketNum, m_ticketCount);
}

void TicketDB::removeTicket(int ticketNum)
{
	MutexLocker locker(&m_mutex);

	m_ticketCount -= ticketNum;
	printf("数据库减少火车票%d张，余量%d张！\n", ticketNum, m_ticketCount);
}

bool TicketDB::buyTicket(const TicketWindow& window, int buyNum)
{
	MutexLocker locker(&m_mutex);
	
	if (buyNum > m_ticketCount)
	{
		printf("窗口%02d出售%d张火车票失败，余量不足只有%d张！\n", window.getWindowID(), buyNum, m_ticketCount);
	}
	else
	{
		m_ticketCount -= buyNum;

		printf("窗口%02d出售%d张火车票成功，余量还有%d张！\n", window.getWindowID(), buyNum, m_ticketCount);
	}
}

TicketDB::TicketDB()
	: m_currTicketNo(10000)
	, m_ticketCount(0)
{
	//初始化互斥量
	pthread_mutex_init(&m_mutex, NULL);
}

TicketWindow::TicketWindow(int windowID)
	: m_windowID(windowID)
	, m_askForExit(false)
{
	pthread_mutex_init(&m_mutex, NULL);
}

TicketWindow::~TicketWindow()
{
	pthread_mutex_destroy(&m_mutex);
}

bool TicketWindow::start()
{
	int ret = pthread_create(&m_thr, NULL, TicketWindow::run, this);
	return (ret == 0);
}

void TicketWindow::askForExit()
{
	MutexLocker locker(&m_mutex);

	m_askForExit = true;
}

void TicketWindow::stop()
{
	int* ret = NULL;
	pthread_join(m_thr, (void**)&ret);

	//pthread_timedjoin_np 超时版本
}

int TicketWindow::getWindowID() const
{
	return m_windowID;
}

void TicketWindow::setWindowID(int id)
{
	m_windowID = id;
}

void* TicketWindow::run(void* arg)
{
	TicketWindow* This = (TicketWindow*)arg;

	while (true)
	{
		//判断是否请求退出
		{
			MutexLocker locker(&This->m_mutex);

			if (This->m_askForExit)
				break;
		}

		srand(time(NULL) + This->getWindowID());
		int buyCount = 1 + rand() % 3;
		TDB.buyTicket(*This, buyCount);
		
		int sleepSec = buyCount;
		sleep(sleepSec);
		//usleep(3000);  N毫秒休眠
	}

	pthread_exit(NULL);
}
