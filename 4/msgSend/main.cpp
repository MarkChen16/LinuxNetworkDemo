#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ipc.h>
#include <sys/msg.h>

#include <unistd.h>

/*
消息：不同进程之间通信

相关函数：ftok  msgget  msgctl  msgsnd  msgrcv
*/

//消息类型
#define MSGTYPE_SAYHI 11

//消息定义
struct msgbuff_t
{
	msgbuff_t()
	{
		mtype = 1;
		memset(message, 0, 256);
		memset(sendpid, 0, 10);
	}

	//消息固定变量
	long mtype;
	
	//自定义部分(最大长度为8192)
	char message[256];
	char sendpid[10];
};

int main(int argc, char* argv[])
{
	int ret = 0;

	system("touch /tmp/msg");
	
	//根据文件的inode值的唯一性和计划编号生成唯一的ipc和键值，文件路径必须存在，进程之间IPC过程中，文件不能删除或者重新创建；
	key_t ipcKey = ftok("/tmp/msg", 'a');
	if (ipcKey == -1)
	{
		printf("Failed to ftok.\n");
		ret = 1;
	}
	else
	{
		//创建消息队列(ipc标识)
		//创建无模式：如果不存在则出错，如果存在则引用
		//IPC_CREAT：如果不存在则创建，如果存在则引用
		//IPC_CREAT | IPC_EXCL：如果不存在则创建，如果存在则报错EEXIST
		int msqID = msgget(ipcKey, IPC_CREAT | 0666);	//IPC_PRIVATE表示系统生成一个
		if (msqID == -1)
		{
			printf("Failed to msgget.\n");
			ret = 2;
		}
		else
		{
			//发送消息
			struct msgbuff_t msgHi;
			msgHi.mtype = MSGTYPE_SAYHI;
			strcpy(msgHi.message, "Hi, Mark Chen!");

			int pid = getpid();
			strcpy(msgHi.sendpid, std::to_string(pid).c_str());

			//IPC_NOWAIT：消息队列已满时，直接返回不写入队列
			int len = sizeof(struct msgbuff_t) - sizeof(long);
			int result = msgsnd(msqID, &msgHi, len, IPC_NOWAIT);
			if (result == -1)
			{
				printf("Failed to msgsnd.\n");
				ret = 2;
			}
			else
			{
				printf("Send Hi Message.\n");
			}
		}
	}

	return ret;
}
