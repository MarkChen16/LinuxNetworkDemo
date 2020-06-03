﻿#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "ticketWindow.h"


/*
多线程：是程序执行路径的不同分支，进程启动只有一个主线程，多线程对资源访问需要同步。

库依赖项：pthread
相关函数：
*/

static void signal_handler(int signo)
{
	if (SIGALRM == signo)
	{
		printf("系统开始关闭售票窗口！\n");
	}
}

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

	//20秒后停止售票
	//signal(SIGALRM, signal_handler);

	alarm(20);
	pause();

	//停止售票
	for (int i = 0; i < 10; i++)
	{
		window[i].askForExit();
		window[i].stop();
	}

	return ret;
}
