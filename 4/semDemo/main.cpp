#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <sys/ipc.h>
#include <sys/sem.h>

/*
信号量：常用于进程间同步访问资源

相关函数：ftok  semget  semctl  semop semtimedop
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

void semAdd(int semID)
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
}

void semSub(int semID)
{
	struct sembuf buf;
	buf.sem_num = 0;
	buf.sem_op = -1;
	buf.sem_flg = 0;

	struct timespec timeOut;
	timeOut.tv_sec = 3;
	timeOut.tv_nsec = 0;
	int result = semtimedop(semID, &buf, 1, &timeOut);
	if (result == -1)
	{
		printf("Timeout, Failed to semSub.\n");
	}
}

void semDel(int semID)
{
	union semun sem;
	sem.val = 0;

	int result = semctl(semID, 0, IPC_RMID, sem);
}

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
		//创建包含10个信号的信号集合
		int semID = semget(keyID, 10, IPC_CREAT | 0666);
		if (semID == -1)
		{
			printf("Failed to semget.\n");
			ret = 2;
		}
		else
		{
			//将编号为0的信号初始化为2
			setSem(semID, 2);
			printf("after init, sem: %d\n", getSem(semID));

			//两次减少
			semSub(semID);
			semSub(semID);
			semSub(semID);
			printf("after sub, sem: %d\n", getSem(semID));

			//三次增加
			semAdd(semID);
			semAdd(semID);
			semAdd(semID);
			semAdd(semID);
			printf("after add, sem: %d\n", getSem(semID));

			//删除编号为0的信号
			semDel(semID);
		}
	}

	return ret;
}
