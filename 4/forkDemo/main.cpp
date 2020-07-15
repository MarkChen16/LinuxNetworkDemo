#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/wait.h>

/*
gdb调试fork子进程：
set follow-fork-mode child; set detach-on-fork on;

set follow-fork-mode parent;
fork后，调试父进程

set follow-fork-mode child;
fork后，调试子进程

set detach-on-fork on;
只调试父进程或者子进程

set detach-on-fork off;
只调试父进程或者子进程，另一个进程进程暂停状态

*/

//数据段
static int g_val = 1;

int main(int argc, char* argv[])
{
	int ret = 0;

	//堆段
	int* h_val = new int();
	*h_val = 2;

	//栈段
	int val = 3;

	//进程分叉：子进程复制父进程的页表，共享代码段，数据段、堆段和栈段，虚拟内存地址都是相同的，通过内核映射程序偏移地址后，指向不同的物理内存地址；
	pid_t pid = fork();
	if (pid == 0)
	{
		//子进程=======================================================
		sleep(15);

		//Copy on write写时复制技术
		//修改数据：任何一个进程修改数据前，进程复制数据所在的页面，同时修改页表；
		g_val += 10;
		printf("子进程 %p: %d\n", &g_val, g_val);

		*h_val += 10;
		printf("子进程 %p: %d\n", h_val, *h_val);

		//先读取、再修改，再读取，虚拟内存地址还是一样的
		printf("子进程 %p: %d\n", &val, val);
		val += 10;
		printf("子进程 %p: %d\n", &val, val);

		_exit(0);
	}
	else if (pid > 0)
	{
		//父进程首先执行============================================

		g_val = 8;	//父进程写时复制，同时修改页表；

		printf("父进程 %p: %d\n", &g_val, g_val);
		printf("父进程 %p: %d\n", h_val, *h_val);
		printf("父进程 %p: %d\n", &val, val);

		int status = 0;
		waitpid(pid, &status, 0);
	}
	else
	{
		printf("Failed to fork.\n");
		ret = 1;
	}

	return ret;
}
