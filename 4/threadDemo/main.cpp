#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ticketWindow.h"

/*
多线程：是程序执行路径的不同分支，进程启动只有一个主线程，多线程对资源访问需要同步。

库依赖项：pthread
pthread_t  pthread_mutex_t

相关函数：
pthread_create  pthread_exit  pthread_join
pthread_mutex_init  pthread_mutex_destory  pthread_mutex_lock  pthread_mutex_unlock
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
