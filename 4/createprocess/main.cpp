#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
fork()：复制父进程，不同的进程ID，写时复制
system()：新建子进程，调用shell外部命令，通过调用fork()、execve()和waitpid()实现
exec()：新建进程代替原进程，使用原进程的资源，进程ID也一样。

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

void do_fork()
{
	//打印父进程的ID
	printf("current pid = %d\n", getpid());

	for (int start = 0; start < 1000; start++)
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

			//修改父进程变量前，复制父进程的内存副本，修改后不会影响父进程的变量
			start = 1000;

			//只有父进程才继续执行循环
			break;
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
