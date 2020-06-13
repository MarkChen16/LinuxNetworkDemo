#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <signal.h>

/*
信号：捕捉系统核心发给进程的信号，比如SIGALRM、SIGKILL、 SIGTERM等等，使用signal()函数注册信号的处理函数；

简单的处理函数：SIG_IGN
signal()、raise()、kill()


更复杂的函数：信号排队，可以设置过滤信号
sigaction()

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

int main(int argc, char* argv[])
{
	int ret = 0;

	//捕捉指定信号
	signal(SIGKILL, SIG_IGN);		//KILL、STOP信号不可捕捉，设置无效
	signal(SIGSTOP, SIG_IGN);

	signal(SIGCHLD, signal_handler);	//子进程结束，wait给子进程收尸
	signal(SIGALRM, signal_handler);	//定时器触发，alarm设置的定时器
	signal(SIGPIPE, signal_handler);			//两次向已关闭的socket写入数据触发，系统默认是退出进程，设置为SIG_IGN后， send将返回-1，erron为EPIPE
	signal(SIGINT, signal_handler);			//Ctrl+C键按下时触发，默认退出进程

	//触发SIGCHLD信号
	//for (int i = 0; i < 3; i++)
	//{
	//	int pid = fork();
	//	if (pid == 0)
	//	{

	//		_exit(0);
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
