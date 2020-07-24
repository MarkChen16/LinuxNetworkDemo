#include "ticketWindow.h"
#include "mutexTicketDB.h"


void TicketDB::addTicket(int ticketNum)
{
	MutexLocker locker(&m_mutexSignal);

	if (ticketNum > 0)
	{
		pthread_cond_broadcast(&m_cond);
	}
}

void TicketDB::waitPutTicket()
{
	MutexLocker locker(&m_mutexWait);

	while (queryTicket() <= 0)
	{
		pthread_cond_wait(&m_cond, &m_mutexWait);	//传入已经上锁的互斥量，等待条件变量被其他线程设置
	}
}

TicketWindow::TicketWindow(TicketDB* TDB, int windowID)
	: m_windowID(windowID)
	, m_askForExit(false)
	, m_TDB(TDB)
{
	assert(m_TDB);
}

TicketWindow::~TicketWindow()
{
}

bool TicketWindow::start()
{
	int ret = pthread_create(&m_thr, NULL, TicketWindow::run, this);
	return (ret == 0);
}

void TicketWindow::askForExit()
{
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

void TicketWindow::waitPutTicket()
{
	m_TDB->waitPutTicket();
}

void TicketWindow::buyTicket(int buyCount)
{
	m_TDB->buyTicket(m_windowID, buyCount);
}

void* TicketWindow::run(void* arg)
{
	TicketWindow* This = (TicketWindow*)arg;

	//等待数据库放票
	This->waitPutTicket();

	while (true)
	{
		//判断是否请求退出
		if (This->m_askForExit)
			break;

		This->buyTicket(1);
	}

	pthread_exit(NULL);
}
