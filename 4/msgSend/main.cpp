#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <iostream>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ipc.h>
#include <sys/msg.h>

#include <unistd.h>
#include <signal.h>
#include <wait.h>

/*
消息：不同进程之间通信

相关函数：ftok  msgget  msgctl  msgsnd  msgrcv
*/

//消息类型定义
enum MsgType
{
	None,
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

	//消息固定变量
	long mtype;

	//自定义部分(最大长度为8192)
	msg_t msg;
};

void sendMsg(int msqId, msgbuff_t& msgBuff);

int main(int argc, char* argv[])
{
	int ret = 0;

	signal(SIGCHLD, SIG_IGN);

	//根据文件的inode值的唯一性和计划编号生成唯一的ipc键值，文件路径必须存在，进程之间IPC过程中，文件不能删除或者重新创建；
	system("touch /tmp/msg");
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
		int msqId = msgget(ipcKey, IPC_CREAT | 0666);
		if (msqId == -1)
		{
			printf("Failed to msgget.\n");
			ret = 2;
		}
		else
		{
			for (int i = 1; i <= 100000; i++)
			{
				pid_t pid = fork();
				if (pid == 0)
				{
					//发送hello消息
					struct msgbuff_t helloMsgBuff;
					helloMsgBuff.mtype = MsgType::Hello;
					strcpy(helloMsgBuff.msg.helloMsg.sender, std::to_string(i).c_str());
					strcpy(helloMsgBuff.msg.helloMsg.message, "Hi, Mark Chen!");

					sendMsg(msqId, helloMsgBuff);

					//发送query消息
					struct msgbuff_t queryMsgBuff;
					queryMsgBuff.mtype = MsgType::Query;
					strcpy(queryMsgBuff.msg.queryMsg.sender, std::to_string(i).c_str());
					queryMsgBuff.msg.queryMsg.queryID = i;

					sendMsg(msqId, queryMsgBuff);

					//发送goodbye消息
					struct msgbuff_t goodbyeMsgBuff;
					goodbyeMsgBuff.mtype = MsgType::GoodByte;
					strcpy(goodbyeMsgBuff.msg.goodbyeMsg.sender, std::to_string(i).c_str());

					sendMsg(msqId, goodbyeMsgBuff);

					exit(0);
				}
			}

			//等待所有子进程结束，发送exit消息
			while (wait(NULL) != -1) {};

			struct msgbuff_t exitMsgBuff;
			exitMsgBuff.mtype = MsgType::Exit;

			sendMsg(msqId, exitMsgBuff);
		}
	}

	return ret;
}

void sendMsg(int msqId, msgbuff_t& msgBuff)
{
	//msgflag默认为0，写入后返回
	//IPC_NOWAIT：消息队列已满时，直接返回不写入队列
	int result = msgsnd(msqId, &msgBuff, sizeof(msgbuff_t) - sizeof(long), 0);
	if (result == -1)
	{
		printf("Failed to msgsnd.\n");
	}
	else
	{
		//printf("success to msgsnd.\n");
	}
}
