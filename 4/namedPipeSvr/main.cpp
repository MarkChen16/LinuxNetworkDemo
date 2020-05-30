#include <stdio.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <unistd.h>
#include <limits.h>

/*
命名管道：调用mkfifo创建命名管道文件，一个进程使用读模式打开，一个进程使用写模式打开来进程通信；
可以用来没有亲属关系的进程之间通信；在使用之前，先用open打开，其他操作跟匿名管道一样; 

相关函数：mkfifo open read write close
*/

int main(int argc, char* argv[])
{
	int ret = 0;
	const char* fifoPath = "/tmp/demofifo";

	//创建管道文件
	int mode = 0666;	//八进制
	mkfifo(fifoPath, mode);

	//打开命名管道
	int fd = open(fifoPath, O_RDONLY);
	if (fd == -1)
	{
		printf("NamedPipe Svr: Failed to open fifo file.\n");
		ret = 2;
	}
	else
	{
		//阻塞读取数据：其他进程打开FIFO写入数据前一直阻塞
		char buff[PIPE_BUF];
		ssize_t nRead = read(fd, buff, PIPE_BUF);
		if (nRead > 0)
		{
			printf("read fifo: %s\n", buff);
		}
		else
		{
			printf("Failed to read data.\n");
			ret = 3;
		}

		//关闭
		close(fd);
	}

	return ret;
}
