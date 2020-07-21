#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ticketWindow.h"

/*
多线程：是程序执行路径的不同分支，进程启动只有一个主线程，多线程对资源访问需要同步。
Linux的线程是轻量级的进程，没有支持挂起、唤醒操作；

库依赖项：pthread
pthread_t  pthread_mutex_t

相关函数：
创建线程 pthread_create  pthread_exit  pthread_join
互斥量同步 pthread_mutex_init  pthread_mutex_lock  pthread_mutex_unlock  pthread_mutex_destory
信号量同步 semget  semop  semctl
*/

int main(int argc, char* argv[])
{
	int ret = 0;

	TDB.addTicket(100);

	//开启10个窗口开始售票
	TicketWindow window[10];
	for (int i = 0; i < 10; i++)
	{
		window[i].setWindowID(i + 1);
		window[i].start();
	}

	//10秒增加50张票
	sleep(10);
	TDB.addTicket(50);

	//5秒后停止售票
	sleep(5);
	for (int i = 0; i < 10; i++)
	{
		window[i].askForExit();
	}

	for (int i = 0; i < 10; i++)
	{
		window[i].stop();
	}

	printf("系统关闭售票窗口！\n");

	return ret;
}
