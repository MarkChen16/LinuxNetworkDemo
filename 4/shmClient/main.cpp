#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/sem.h>

//2.6内核已经注释了semun的定义，需要在程序里面自行定义
union semun
{
	int val;
	struct semid_ds* buf;
	unsigned short* array;
	struct seminfo* __buf;
} sem_union;

//数据结构
const int BUFF_LEN = 100 * 1024 * 1024;
struct shm_data_t
{
	int pid;
	char buff[BUFF_LEN];	//100M大小
};

int main(int argc, char argv[])
{
	int ret = 0;

	//创建key值
	system("touch /tmp/shmDemo");
	system("touch /tmp/shmDemo_sem");

	key_t keyShm = ftok("/tmp/shmDemo", 'a');
	key_t keySem = ftok("/tmp/shmDemo_sem", 'a');
	if (keyShm == -1 || keySem == -1)
	{
		printf("Fail to create key; \n");
		ret = 1;
	}
	else
	{
		//以读写模式获取共享内存
		int shmID = shmget(keyShm, sizeof(shm_data_t), IPC_CREAT | 0666);
		int semID = semget(keySem, 1, IPC_CREAT | 0666);
		if (shmID == -1 || semID == -1)
		{
			printf("Fail to create shm or sem; \n");
			ret = 2;
		}
		else
		{
			//映射共享内存，以读写模式映射
			shm_data_t* shareData = (shm_data_t*)shmat(shmID, NULL, 0);

			//写入数据
			if (shareData)
			{
				shareData->pid = getpid();
				memset(shareData->buff, 0xAA, BUFF_LEN);
			}
			else
			{
				printf("Fail to attch adress.\n");
			}

			//断开共享内存映射
			shmdt(shareData);

			//信号量减1，当信号量为0时，阻塞等待
			struct sembuf semData = { 0, -1, 0 };
			semop(semID, &semData, 1);
		}
	}

	return ret;
}
