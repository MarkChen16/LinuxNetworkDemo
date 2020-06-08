#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>
#include <sys/wait.h>

/*
fork()：创建子进程
system()：新建子进程，调用shell外部命令，通过调用fork()、execve()和waitpid()实现
exec()：新建进程代替原进程，使用原进程的资源，进程ID也一样。

僵尸进程产生的原因：
父进程fork了大量的子进程，一直在运行但没有执行wait/waitpid处理结束的子进程，导致pid标识耗尽，下一次fork失败；
如果父进程先于子进程结束，init进程会接管所有的子进程，不会产生僵尸进程；

除了init进程，所有进程都是有父进程的，所有进程都是复制或者克隆的
使用pstree查看所有进程的父子关系
*/

void do_fork();
void do_system();
void do_exec();

int main(int argc, char* argv[])
{
	int ret = 0;

	if (argc != 2)
	{
		printf("Error: vailed argc.\n");
		ret = 1;
	}
	else
	{
		int createType = atoi(argv[1]);

		if (createType == 1)
		{
			do_fork();
		}
		else if (createType == 2)
		{
			do_system();
		}
		else
		{
			do_exec();
		}
	}

	return ret;
}

static void signal_handler(int signo)
{
	if (signo == SIGCHLD)
	{
		//给当前进程的所有已结束的子进程收尸
		for (;;)
		{
			pid_t childPid = wait(NULL);

			if (childPid != -1)
				printf("SIGCHLD - collect child pid: %d\n", childPid);
			else
				break;
		}
	}
}

void do_fork()
{
	//僵尸进程解决方法1：忽略SIGCHLD信号，当子进程结束后，会立即释放数据结构(PID，进程状态，进程返回值)，避免出现大量僵尸进程，导致pid标识耗尽，fork创建子进程失败；
	//使用ps -el命令查看僵尸进程，状态为Z的进程
	//signal(SIGCHLD, SIG_IGN);

	//僵尸进程解决方法2：捕捉SIGCHLD信号，使用wait/waitpid来为子进程收尸；
	signal(SIGCHLD, signal_handler);

	//打印父进程的ID
	printf("current pid = %d\n", getpid());

	for (int start = 0; start < 10000; start++)
	{
		pid_t pid = fork();
		if (pid > 0)
		{
			//父进程
			printf("fork new process = %d\n", pid);
		}
		else if (pid == 0)
		{
			//子进程：处理任务，start变量是复制父进程的内存副本
			int i = 0;

			printf("pid(%d) ppid(%d):", getpid(), getppid());

			for (i = start; i < 1000; i++)
			{
				printf("%d ", i);
			}
			printf("\n");

			//写时复制：修改父进程变量时，复制父进程的内存页，修改后不会影响父进程的变量
			//但子进程和父进程的虚拟地址是一样，但通过内核映射到物理内存地址是不一样的；
			start = 1000;

			_exit(0);
		}
		else
		{
			//复制进程失败
			printf("Failed to fork.\n");

			break;
		}
	}
}

void do_system()
{
	int ret = system("ping www.baidu.com -c 5");
	printf("ret = %d\n", ret);
}

void do_exec()
{
	char* argv[] = {"/bin/ls", "-l", NULL};

	if (execve("/bin/ls", argv, NULL) < 0)
	{
		printf("Failed to execve.\n");
	}
	else
	{
		//成功后不会返回，因为新进程已经代替原进程了，这里不会执行
		//printf("Success to execve.\n");
	}
}
