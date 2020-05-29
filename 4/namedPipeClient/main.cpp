#include <stdio.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <unistd.h>
#include <limits.h>

#include <string.h>

int main(int argc, char* argv[])
{
	int ret = 0;
	const char* fifoPath = "/tmp/demofifo";

	//创建管道文件
	int mode = 0666;	//八进制
	mkfifo(fifoPath, mode);

	//打开命名管道
	int fd = open(fifoPath, O_WRONLY);  //没有使用O_NONBLOCK
	if (fd == -1)
	{
		printf("NamedPipe Client: Failed to open fifo file.\n");
		ret = 2;
	}
	else
	{
		//阻塞写入数据：写入数据后，等待其他进程打开FIFO读取才返回
		const char* buff = "1234567890";
		ssize_t nWrite = write(fd, buff, strlen(buff));
		if (nWrite > 0)
		{
			printf("write fifo: %s\n", buff);
		}
		else
		{
			printf("Failed to write data.\n");
			ret = 3;
		}

		//关闭
		close(fd);
	}

	return ret;
}
