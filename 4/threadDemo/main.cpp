#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ticketWindow.h"
#include "mutexTicketDB.h"
#include "spinTicketDB.h"
#include "atomicTicketDB.h"
#include "rwlockTicketDB.h"

/*
多线程：是程序执行路径的不同分支，进程启动只有一个主线程，多线程对资源访问需要同步。
Linux的线程是轻量级的进程，没有支持挂起、唤醒操作；

库依赖项：pthread
pthread_t  pthread_mutex_t

相关函数：
创建线程 pthread_create  pthread_exit  pthread_join

线程同步机制：
互斥量pthread_mutex_t： pthread_mutex_init  pthread_mutex_lock  pthread_mutex_unlock  pthread_mutex_destory
读写锁pthread_rwlock_t：  pthread_rwlock_init  pthread_rwlock_rdlock  pthread_rwlock_wrlock 	pthread_rwlock_unlock  pthread_rwlock_destroy
自旋锁spinlock_t： pthread_spin_init  pthread_spin_lock  pthread_spin_unlock
原子变量std::atomic：  compare_exchange_strong
条件变量pthread_cond_t： pthread_cond_init  pthread_cond_wait  pthread_cond_signal  pthread_cond_broadcast  pthread_cond_destroy

注意： 
自旋锁：属于busy-waitting模式，占有CPU使用率比较高，效率也比较高；其他锁是sleep-waitting模式；
条件变量：等待A线程完成某件事件之后，线程B才开始继续运行，条件变量需要跟互斥量配合使用；

*/

#define TICKET_WINDOWS_MAXCOUNT 1000

int main(int argc, char* argv[])
{
	int ret = 0;

	int ticketDBType = 1;
	int ticketWindowCount = 10;

	//使用那种同步机制的数据库
	if (argc >= 2)
	{
		ticketDBType = atoi(argv[1]);
	}
	
	//指定售票窗口数量
	if (argc >= 3)
	{
		ticketWindowCount = atoi(argv[2]);
	}

	if (ticketWindowCount > TICKET_WINDOWS_MAXCOUNT)
	{
		ticketWindowCount = TICKET_WINDOWS_MAXCOUNT;
	}

	//初始化火车票数据库
	TicketDB* pTicketDB = NULL;

	if (ticketDBType == 1)
		pTicketDB = MTDB;
	else if (ticketDBType == 2)
		pTicketDB = STDB;
	else if (ticketDBType == 3)
		pTicketDB = ATDB;
	else if (ticketDBType == 4)
		pTicketDB = RWTDB;

	//开启N个窗口开始售票
	TicketWindow* window[TICKET_WINDOWS_MAXCOUNT];
	for (int i = 0; i < ticketWindowCount; i++)
	{
		window[i] = new TicketWindow(pTicketDB, i);
		if (!window[i]->start())
		{
			perror("start");
		}
	}

	//开始放票，倒计时开始
	for (int i = 3; i >= 1; i--)
	{
		sleep(1);
		printf("开始放票(%i)...\n", i);
	}

	pTicketDB->addTicket(1000000);

	//10秒后停止售票
	sleep(10);

	for (int i = 0; i < ticketWindowCount; i++)
	{
		window[i]->askForExit();
	}

	for (int i = 0; i < ticketWindowCount; i++)
	{
		if (window[i] != NULL)
		{
			window[i]->stop();

			delete window[i];
			window[i] = NULL;
		}
	}

	printf("系统停止售票！\n");

	return ret;
}
