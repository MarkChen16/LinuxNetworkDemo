#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

/*
共享内存：进程间共享内存空间，地址指向同一块内存片段，是所有IPC中速度最快的，常常与信号量配合使用；
shmat和shmdt采用计数方式引用，当计数为0时，系统就会释放共享内存；

相关函数：shmget shmat shmdt shmctl
*/

//2.6内核已经注释了semun的定义，需要在程序里面自行定义
union semun
{
	int val;
	struct semid_ds* buf;
	unsigned short* array;
	struct seminfo* __buf;
} sem_union;

int main(int argc, char* argv[])
{
	int ret = 0;
	
	system("touch /tmp/shm");
	key_t keyID = ftok("/tmp/shm", 'a');
	if (keyID == -1)
	{
		printf("Failed to ftok.\n");
		ret = 1;
	}
	else
	{
		int shmID = shmget(keyID, 1024, IPC_CREAT | 0666);
		int semID = semget(keyID, 1, IPC_CREAT | 0666);

		if (shmID == -1 || semID == -1)
		{
			printf("Failed to shmget & semget.\n");
			ret = 2;
		}
		else
		{
			//父进程获取共享内存写指针
			char* shmwAddr = (char*)shmat(shmID, 0, 0);
			assert(shmwAddr);

			const char* message = "您好，共享内存！";
			memcpy(shmwAddr, message, strlen(message) + 1);

			//初始化信号量为0
			union semun sem;
			sem.val = 0;
			int result = semctl(semID, 0, SETVAL, sem);
			assert(result == 0);

			for (int i = 0; i < 100; i++)
			{
				//进程分叉======================
				pid_t pidID = fork();
				if (pidID == 0)
				{
					//子进程：读取共享内存
					char* shmrAddr = (char*)shmat(shmID, 0, 0);
					assert(shmrAddr);

					printf("Proc(%d): %s\n", getpid(), shmrAddr);
					
					shmdt(shmrAddr);

					//资源解锁
					struct sembuf buf = { 0, -1, 0 };
					semop(semID, &buf, 1);

					_exit(0);
				}
				else if (pidID > 0)
				{
					//父进程
					printf("Create Proc: %d\n", pidID);

					//资源加锁
					struct sembuf buf = { 0, +1, 0 };
					semop(semID, &buf, 1);
				}
				else
				{
					printf("Failed to fork.\n");
					ret = 3;
				}
			}

			//等待信号量为0
			struct sembuf buf = {0, 0, 0};
			semop(semID, &buf, 1);

			//删除共享内存写指针
			shmdt(shmwAddr);
		}
	}

	return ret;
}
