#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <stdlib.h>
#include <wait.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <sys/ipc.h>
#include <sys/sem.h>

#include <semaphore.h>

/*
信号量：通常用于进程间同步访问资源

相关函数：ftok  semget  semctl  semop semtimedop

另一个信号量的实现 semaphore.h - sem_t sem_init  sem_wait  sem_post
*/

//2.6内核已经注释了semun的定义，需要在程序里面自行定义
union semun
{
	int val;
	struct semid_ds* buf;
	unsigned short* array;
	struct seminfo* __buf;
} sem_union;

//获取信号量
void setSem(int semID, int val)
{
	union semun sem;
	sem.val = val;
	
	int result = semctl(semID, 0, SETVAL, sem);
}

int getSem(int semID)
{
	return semctl(semID, 0, GETVAL);
}

bool semAdd(int semID)
{
	struct sembuf buf;
	buf.sem_num = 0;
	buf.sem_op = +1;
	buf.sem_flg = 0;	//使用SEM_UNDO，进程退出或者异常退出时，系统会做P\V操作，避免资源死锁

	//sem_op > 0 表示释放资源
	//sem_op = 0 表示等待资源全部用完，没有设置IPC_NOWAIT的话就一直阻塞，直到信号为0
	//sem_op < 0 表示获取资源，如果资源不够，没有设置IPC_NOWAIT就会一直阻塞
	//可以使用semtimedop，可以设置超时

	int result = semop(semID, &buf, 1);
	if (result == -1)
	{
		return false;
	}

	return true;
}

bool semSub(int semID)
{
	struct sembuf buf;
	buf.sem_num = 0;
	buf.sem_op = -1;
	buf.sem_flg = 0;

	//超时设置为5秒
	struct timespec timeOut;
	timeOut.tv_sec = 5;
	timeOut.tv_nsec = 0;
	int result = semtimedop(semID, &buf, 1, &timeOut);
	if (result == -1)
	{
		printf("Timeout, Failed to semSub.\n");
		return false;
	}

	return true;
}

void semDel(int semID)
{
	union semun sem;
	sem.val = 0;

	int result = semctl(semID, 0, IPC_RMID, sem);
}

//父子进程使用信号量同步例子
int main(int argc, char* argv[])
{
	int ret = 0;

	//创建文件
	system("touch /tmp/sem");

	//使用文件的inode生成键值
	key_t keyID = ftok("/tmp/sem", 'a');
	if (keyID == -1)
	{
		printf("Failed to ftok.\n");
		ret = 1;
	}
	else
	{
		//创建总量为10个信号的信号集合
		int semID = semget(keyID, 10, IPC_CREAT | 0666);
		if (semID == -1)
		{
			printf("Failed to semget.\n");
			ret = 2;
		}
		else
		{
			//信号量初始化为0
			setSem(semID, 0);

			pid_t pid = fork();
			if (pid > 0)
			{
				//父进程：3秒钟之后设置信号量
				sleep(3);
				semAdd(semID);

				//等待子进程退出
				int status = 0;
				waitpid(pid, &status, 0);

				//删除编号为0的信号
				semDel(semID);
			}
			else if (pid == 0)
			{
				//子进程
				printf("Child[%d] start...\n", getpid());

				//如果没有资源了，会等待直到超时
				if (semSub(semID))
				{
					//处理资源
					printf("Child[%d] handle...\n", getpid());
				}

				printf("Child[%d] end...\n", getpid());

				_exit(0);
			}
			else
			{
				printf("Child[%d] start...\n", getpid());
			}
		}
	}

	return ret;
}
