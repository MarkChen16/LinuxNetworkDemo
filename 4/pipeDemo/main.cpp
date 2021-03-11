#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <wait.h>

/*
pipe匿名管道：
1、半双工，数据在同一时刻只能在一个方向上流动，需要两个匿名管道才能完成全双工通信。
2、数据只能从管道的一端写入，从另一端读出。
3、写入管道中的数据遵循先入先出的规则。
4、管道所传送的数据是无格式的，这要求管道的读出方与写入方必须事先约定好数据的格式，如多少字节算一个消息等。
5、管道不是普通的文件，不属于某个文件系统，其只存在于内存中。
6、管道在内存中对应一个缓冲区。不同的系统其大小不一定相同。
7、从管道读数据是一次性操作，数据一旦被读走，它就从管道中被抛弃，释放空间以便写更多的数据。
8、管道没有名字，只能在具有公共祖先的进程（父进程与子进程，或者两个兄弟进程，具有亲缘关系）之间使用 (非常重点)。
9、管理的默认大小是64KB(65536)，可以使用ulimit -p [size]临时修改；

阻塞情况：
1、当管道是空的，消费进程read时阻塞； 
2、当管理是满的，生产进程write时阻塞； 

*/

struct pipe_data_t
{
	int pid;
	char msg[1024];
};

int main(int argc, char* argv[])
{
	int ret = 0;

	//创建两个匿名管道实现全双工通信
	int result = 0;
	int toFatherFD[2] = {0, 0};
	int toChildFD[2] = {0, 0};

	result = pipe(toFatherFD);
	if (result == -1)
	{
		printf("Failed to pipe.\n");
		return ret;
	}

	result = pipe(toChildFD);
	if (result == -1)
	{
		printf("Failed to pipe.\n");
		return ret;
	}

	pid_t pid = fork();
	if (pid == 0)
	{
		close(toFatherFD[0]);  //关闭流向父进程的读FD
		close(toChildFD[1]);   //关闭流向子进程的写FD

		//写入数据
		pipe_data_t writeData;
		writeData.pid = getpid();
		strcpy(writeData.msg, "say hello...");

		int lenWrite = write(toFatherFD[1], &writeData, sizeof(pipe_data_t));
		if (lenWrite == -1)
		{
			printf("Failed to write.\n");
		}

		//读取数据
		pipe_data_t readData;
		int lenRead = read(toChildFD[0], &readData, sizeof(pipe_data_t));
		if (lenRead == -1)
		{
			printf("Failed to read.\n");
		}
		else
		{
			printf("Father[%i]: %s\n", readData.pid, readData.msg);
		}
		
		close(toFatherFD[1]);
		close(toChildFD[0]);

		_exit(0);
	}
	else if (pid > 0)
	{
		close(toFatherFD[1]);  //关闭流向父进程的写FD
		close(toChildFD[0]);   //关闭流向子进程的读FD

		pipe_data_t readData;
		pipe_data_t writeData;
		writeData.pid = getpid();
		strcpy(writeData.msg, "hello!!!");

		//读取数据
		int lenRead = read(toFatherFD[0], &readData, sizeof(pipe_data_t));
		if (lenRead == -1)
		{
			printf("Failed to read.\n");
		}
		else
		{
			printf("Child[%i]: %s\n", readData.pid, readData.msg);
		}

		//写入数据
		int lenWrite = write(toChildFD[1], &writeData, sizeof(pipe_data_t));
		if (lenWrite == -1)
		{
			printf("Failed to write.\n");
		}

		close(toFatherFD[0]);
		close(toChildFD[1]);
	}
	else if (pid < 0)
	{
		printf("Failed to fork.\n");
	}

	return ret;
}
