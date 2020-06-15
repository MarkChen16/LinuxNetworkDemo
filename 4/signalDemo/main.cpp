#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <signal.h>
#include <bits/sigthread.h>

/*
信号：捕捉系统核心发给进程的信号，比如SIGALRM、SIGKILL、 SIGTERM等等，使用signal()函数更改信号处理函数；

修改程序的信号处理函数
signal()：异步中断，会被重置
sigaction()：异步中断，不会被重置

每个线程都有信号屏蔽集，子线程从父线程继承信号集
主线程屏蔽信号：
sigprocmask()

多线程屏蔽信号：
pthread_sigmask()

*/

static void signal_handler(int signo)
{
	if (signo == SIGALRM)
	{
		//定时器信号
		printf("get signal: SIGALRM\n");
	}
	else if (signo == SIGINT)
	{
		//INT信号
		printf("get signal: SIGINT\n");
	}
	else if (signo == SIGCHLD)
	{
		//子进程退出
		printf("get signal: SIGCHLD\n");
	}
	else
	{
		//其他信号
		printf("get else signal: %d\n", signo);
	}
}

//进程屏蔽信号
void blockProSig(int signo)
{
	sigset_t sigMask;
	sigemptyset(&sigMask);
	sigaddset(&sigMask, signo);

	sigprocmask(SIG_BLOCK, &sigMask, NULL);
}

//屏蔽线程信号
void blockThreadSig(int signo)
{
	sigset_t sigMask;
	sigemptyset(&sigMask);
	sigaddset(&sigMask, signo);

	//pthread_sigmask(SIG_BLOCK, &sigMask, NULL);		//找不到定义
}

int main(int argc, char* argv[])
{
	int ret = 0;

	//捕捉指定信号
	//signal(SIGKILL, SIG_IGN);		//KILL、STOP信号不可捕捉，设置无效
	//signal(SIGSTOP, SIG_IGN);

	signal(SIGCHLD, signal_handler);	//子进程结束，wait给子进程收尸
	signal(SIGALRM, signal_handler);	//定时器触发，alarm设置的定时器
	//signal(SIGPIPE, signal_handler);			//两次向已关闭的socket写入数据触发，系统默认是退出进程，设置为SIG_IGN后， send将返回-1，erron为EPIPE
	signal(SIGPIPE, SIG_IGN);			//两次向已关闭的SOCKET写入数据时触发，默认退出进程

	//使用signal注册SIGINT处理函数
	//signal(SIGINT, signal_handler);			//Ctrl+C键按下时触发，默认退出进程

	//使用sigaction注册SIGINT处理函数
	//struct sigaction intAct;
	//intAct.sa_handler = signal_handler;
	//sigemptyset(&intAct.sa_mask);
	//sigaddset(&intAct.sa_mask, SIGINT);		//执行信号时屏蔽SIGINT信号
	//sigaction(SIGINT, &intAct, NULL);
	
	//sigprocmask屏蔽进程信号
	//blockProSig(SIGINT);

	//触发SIGCHLD信号
	for (int i = 0; i < 3; i++)
	{
		int pid = fork();
		if (pid == 0)
		{

			_exit(EXIT_SUCCESS);
		}
	}

	//向当前进程发送kill信号
	//raise(SIGKILL);
	//kill(getpid(), SIGKILL);

	//触发定时器
	printf("Exit at 10 seconds...\n");
	alarm(10);

	pause();

	return ret;
}
