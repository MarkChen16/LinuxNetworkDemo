#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <memory.h>
#include <assert.h>

#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <sys/ipc.h>
#include <sys/msg.h>

//查询消息队列最大数量、单条消息最大字节数和消息队列最大字节数
//ipcs		查询所有ipc使用情况
//ipcs -u	显示ipc资源已使用情况
//ipcs -l	显示ipc资源的限制
//ipcs -p	显示ipc最后操作pid

/*
修改ipc资源的最大限制 /etc/sysctl.conf，重启后生效

------ Messages: Limits --------
max queues system wide = 3716
max size of message (bytes) = 65536
default max size of queue (bytes) = 272000000

*/

/*
在/etc/rc.d/rc.local脚本文件增加以下命令
echo 272000000 > /proc/sys/kernel/msgmnb;

*/

//消息类型定义
enum MsgType
{
	None = 1,
	Hello,
	Query,
	GoodByte,
	Exit
};

//消息体定义
struct msg_hello_t
{
	char sender[10];
	char message[256];
};

struct msg_query_t
{
	char sender[10];
	int queryID;
};

struct msg_goodbye_t
{
	char sender[10];
};

//消息数据结构
struct msgbuff_t
{
	union msg_t
	{
		msg_hello_t helloMsg;
		msg_query_t queryMsg;
		msg_goodbye_t goodbyeMsg;
	};

	msgbuff_t()
	{
		mtype = None;
		memset(&msg, 0, sizeof(msg_t));

		assert(sizeof(msg_t) < 8192);
	}

	//消息类型，固定字段，必须大于0
	long mtype;

	//自定义部分(最大长度为8192)
	msg_t msg;
};

int main(int argc, char* argv[])
{
	int ret = 0;

	//根据文件的inode值的唯一性和计划编号生成唯一的ipc键值，文件路径必须存在，进程之间IPC过程中，文件不能删除或者重新创建，否则inode会改变；
	system("touch /tmp/msg");
	key_t ipcKey = ftok("/tmp/msg", 'a');
	if (ipcKey == -1)
	{
		printf("Failed to ftok.\n");
		ret = 1;
	}
	else
	{
		//ipcKey也可以为IPC_PRIVATE，表示是一个私有IPC，仅在父子进程之间使用
		int msqID = msgget(ipcKey, IPC_CREAT | 0666);
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

				//__msgtyp为0时，读取消息队列中第一条消息返回；大于0时，读取消息队列中第一条类型为msgtype的消息返回；
				//__msgflg默认阻塞读取消息，一直等到有消息才返回，设置为IPC_NOWAIT，没有消息则马到返回错误
				struct msgbuff_t msgData;
				result = msgrcv(msqID, &msgData, sizeof(struct msgbuff_t), 0, 0);
				if (result == -1)
				{
					printf("Failed to msgrcv.\n");
					ret = 2;

					break;
				}
				else
				{
					switch (msgData.mtype)
					{
					case MsgType::Hello:
						printf("%s login: %s\n", msgData.msg.helloMsg.sender, msgData.msg.helloMsg.message);
						break;
					case MsgType::Query:
						printf("%s query id: %i\n", msgData.msg.queryMsg.sender, msgData.msg.queryMsg.queryID);
						break;
					case MsgType::GoodByte:
						printf("%s say goodbye.\n", msgData.msg.goodbyeMsg.sender);
						break;
					case MsgType::Exit:
						printf("system exit.\n");
						break;
					default:
						printf("unknown message type is %i.", msgData.mtype);
						break;
					}

					if (msgData.mtype == MsgType::Exit)
						break;
				}
			}
		}
	}

	return ret;
}
