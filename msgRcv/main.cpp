#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <memory.h>

#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <sys/ipc.h>
#include <sys/msg.h>

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

	//自定义部分
	char message[256];
	char sendpid[10];
};

int main(int argc, char* argv[])
{
	int ret = 0;

	system("touch /tmp/msg");

	key_t ipcKey = ftok("/tmp/msg", 'a');
	if (ipcKey == -1)
	{
		printf("Failed to ftok.\n");
		ret = 1;
	}
	else
	{
		int msqID = msgget(ipcKey, IPC_CREAT | 0666);	//IPC_PRIVATE表示系统生成一个
		if (msqID == -1)
		{
			printf("Failed to msgget.\n");
			ret = 2;
		}
		else
		{
			
			while (true)
			{
				int result = 0;

				//查询消息队列状态
				struct msqid_ds msg_stat;
				result = msgctl(msqID, IPC_STAT, &msg_stat);
				if (result == -1)
				{
					printf("Failed to msgrcv.\n");
					ret = 3;

					break;
				}
				else
				{
					printf("msg count: %d\n", msg_stat.msg_qnum);
				}

				//阻塞读取消息，一直等到有消息才返回
				struct msgbuff_t msgHi;
				result = msgrcv(msqID, &msgHi, sizeof(struct msgbuff_t), MSGTYPE_SAYHI, IPC_NOWAIT);
				if (result == -1)
				{
					printf("Failed to msgrcv.\n");
					ret = 2;

					break;
				}
				else
				{
					printf("Message(%s): %s\n", msgHi.sendpid, msgHi.message);
				}
			}
		}
	}

	return ret;
}
