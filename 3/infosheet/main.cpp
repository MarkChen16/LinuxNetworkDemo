#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

/*
命令xinfo [-p] [-r] [-w]
参数说明：
-p：打印所有记录
-r：读取指定的记录
-w：写入新的记录

使用open close fstat lseek read write mmap munmap fcntl ioctl

lseek函数：可以移动当前读写位置，还可以创建空洞文件
空洞文件的用途：
1、进程通信内存映射
2、提前分配连续的磁盘空间，减少读写时寻道开销
3、下载文件提前分配好文件空间，多线程快速写入数据

mmap和munmap函数：两个进程对同一个空洞文件创建内存映射，实现进程间通信

fcntl函数：获取和设置已经打开文件的性质
int fcntl(int fd, int cmd);
int fcntl(int fd, int cmd, long arg);
int fcntl(int fd, int cmd struct flock* lock);

cmd参数类型：
复制设备描述符
F_DEPFD

获取和设置设备描述符
F_GETFD
F_SETFD

获取和设置文件的状态
F_GETFL
F_SETFL

用于IO消息驱动，设置处理IO消息的进程ID或者进程组ID
F_GETOWN
F_SETOWN

获取和设置记录锁
F_GETLK
F_SETLK

获取和设置文件租约
F_GETLEASE
F_SETLEASE

用户空间的ioctl函数：直接发送特殊命令给设备驱动
int ioctl(int fd, int request, ...);


*/

#define NAME_MAX_LEN 30

struct StudInfo
{
	StudInfo()
	{
		id = 0;
		memset(name, 0, NAME_MAX_LEN);
		age = 0;
		score = 0;
	}

	void print()
	{
		printf("%d\t%s\t%d\t%d\n", id, name, age, score);
	}

	int id;
	char name[NAME_MAX_LEN];
	int age;
	int score;
};

int main(int argc, char* argv[])
{
	int ret = 0;

	//读取参数
	const char* params = "-p";
	if (argc >= 2)
		params = argv[1];

	const char* infoFile = "stud.info";
	int fd = open(infoFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if (fd == -1)
	{
		printf("Failed to open %s\n", infoFile);
		ret = 1;
	}
	else
	{
		struct stat st;
		
		if (fstat(fd, &st) == -1)
		{
			printf("Failed to get stat.\n");
			ret = 2;
		}
		else
		{
			int countInfo = st.st_size / sizeof(StudInfo);

			//获取文件访问模式
			int flags = fcntl(fd, F_GETFL, 0);
			flags = flags & O_ACCMODE;	//获取标志组合中的访问模式

			//获取接收消息的进程ID
			int uid = fcntl(fd, F_GETOWN);
			int rev1 = fcntl(fd, F_SETOWN, 1000);

			//获取接收缓冲区的字节数
			int nReadBuffCount = 10;
			int rev2 = ioctl(fd, FIONREAD, &nReadBuffCount);

			//判断参数
			if (strcmp(params, "-p") == 0 && argc == 2)
			{
				//打印所有记录=========================
				StudInfo* ptrInfo = NULL;

				//映射到内存，使用指针操作，速度更快
				ptrInfo = (StudInfo*)mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
				if (ptrInfo == (StudInfo*)-1)
				{
					printf("Failed to map address.\n");
					ret = 3;
				}
				else
				{
					for (int i = 0; i < countInfo; i++)
					{
						ptrInfo[i].print();
					}

					//取消映射
					munmap(ptrInfo, st.st_size);
				}
			}
			else if (strcmp(params, "-r") == 0 && argc == 3)
			{
				//读取指定的记录=========================
				int readIndex = atoi(argv[2]) - 1;

				if (readIndex < 0 || readIndex >= countInfo)
				{
					printf("Read index is vaild.\n");
					ret = 5;
				}
				else
				{
					//移动到指定位置
					if (lseek(fd, readIndex*sizeof(StudInfo), SEEK_SET) == -1)
					{
						printf("Failed to lseek.\n");
						ret = 6;
					}
					else
					{
						StudInfo readInfo;

						ssize_t readLen = read(fd, &readInfo, sizeof(StudInfo));
						if (readLen == -1)
						{
							printf("Failed to read.\n");
							ret = 7;
						}
						else
						{
							readInfo.print();
						}
					}
				}
			}
			else if (strcmp(params, "-w") == 0 && argc == 6)
			{
				//写入新的记录=========================
				StudInfo info;

				info.id = atoi(argv[2]);	//执行man atoi查询用法
				strncpy(info.name, argv[3], strlen(argv[3]) < NAME_MAX_LEN ? strlen(argv[3]) : NAME_MAX_LEN - 1);
				info.age = atoi(argv[4]);
				info.score = atoi(argv[5]);

				//移动到最后的位置
				if (lseek(fd, 0, SEEK_END) == -1)
				{
					printf("Failed to lseek.\n");
					ret = 4;
				}
				else
				{
					//写入新的记录
					ssize_t writeCount = write(fd, &info, sizeof(StudInfo));
					if (writeCount == -1)
					{
						printf("Failed to write.\n");
						ret = 5;
					}

					//让系统把缓冲区的数据写入文件
					fsync(fd);
				}
			}
			else
			{
				printf("Error: Invalid parameters\n");
				ret = 1;
			}
		}

		close(fd);
	}

	return ret;
}
