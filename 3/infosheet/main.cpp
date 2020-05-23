#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>

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

/*
命令xinfo [-p] [-r] [-w]
参数说明：
-p：打印所有记录
-r：读取指定的记录
-w：写入新的记录

使用open close fstat lseek read write mmap munmap fcntl

*/

int openFile();
void closeFile();

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

			//获取文件状态
			int flags = fcntl(fd, F_GETFL, 0);
			flags = flags & O_ACCMODE;	//获取组合中的访问模式

			//判断参数
			if (strcmp(params, "-p") == 0 && argc == 2)
			{
				//打印所有记录=========================
				StudInfo* ptrInfo = NULL;

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
