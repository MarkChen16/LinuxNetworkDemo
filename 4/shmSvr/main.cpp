#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/sem.h>

/*
共享内存：进程间共享内存空间，地址指向同一块内存片段，是所有IPC中速度最快的，常常与信号量配合使用；
shmat和shmdt采用计数方式引用，当计数为0时，系统就会释放共享内存；

相关函数：shmget shmat shmdt shmctl

修改共享内存的最大字节数，/etc/sysctl.conf永久生效；默认最大为4G，但32位用户空间的虚拟内存地址大小只有3G；
*/

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

int main(int argc, char* argv[])
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
		//创建内存共享和信号量，0666表示拥有用户、用户所在组和其他用户的进程都可以读写
		int shmID = shmget(keyShm, sizeof(shm_data_t), IPC_CREAT | 0666);
		int semID = semget(keySem, 1, IPC_CREAT | 0666);	//信号数量为1
		if (shmID == -1 || semID == -1)
		{
			printf("Fail to create shm or sem; \n");
			ret = 2;
		}
		else
		{
			//设置编号为0的信号量为1
			semun semVal;
			semVal.val = 1;
			semctl(semID, 0, SETVAL, semVal);

			//等待信号量为0，超时为3秒
			struct sembuf semData = { 0, 0, 0 };
			semop(semID, &semData, 1);

			//timespec ts;
			//ts.tv_sec = 3;
			//semtimedop(semID, &semData, 1, &ts);

			//映射共享内存，只读模式
			const shm_data_t* shareData = (const shm_data_t*)shmat(shmID, NULL, SHM_RDONLY);
			if (shareData)
			{
				printf("100M Data has arrived, pid is %i.\n", shareData->pid);
			}
			else
			{
				printf("Fail to attch to address.\n");
			}

			//断开共享内存映射，使内存映射的计数减1，当计数为0，内核将释放这块内存
			shmdt(shareData);

			//删除共享内存
			shmctl(shmID, IPC_RMID, NULL);

			//删除信号量
			semctl(semID, 1, IPC_RMID);
		}
	}

	return ret;
}
