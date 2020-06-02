#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <signal.h>

/*
信号：捕捉系统核心发给进程的信号，比如SIGALRM、SIGKILL、 SIGTERM等等，使用signal()函数注册信号的处理函数；

相关函数：signal()、raise()、kill()

*/

static void signal_handler(int signo)
{
	if (signo == SIGALRM)
	{
		//定时器信号
		printf("get signal: SIGALRM\n");
	}
	else if (signo == SIGKILL)
	{
		//kill信号
		printf("get signal: SIGKILL\n");
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

int main(int argc, char* argv[])
{
	int ret = 0;

	signal(SIGCHLD, signal_handler);
	signal(SIGKILL, signal_handler);
	signal(SIGALRM, signal_handler);

	//触发SIGCHLD信号
	//for (int i = 0; i < 3; i++)
	//{
	//	int pid = fork();
	//	if (pid == 0)
	//	{

	//		return 0;
	//	}
	//}

	//向当前进程发送kill信号
	//raise(SIGKILL);
	//kill(getpid(), SIGKILL);

	//触发定时器
	printf("Exit at 5 seconds...\n");
	alarm(5);

	pause();

	return ret;
}
