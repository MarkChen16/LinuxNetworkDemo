﻿#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>

#include <sys/wait.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define HOST_PORT 1800

int main(int argc, char* argv[])
{
	int ret = 0;
	int clientCount = 100000;
	const char* remoteIP = "127.0.0.1";

	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	if (argc >= 2)
		remoteIP = argv[1];

	if (argc >= 3)
		clientCount = atoi(argv[2]);

	sockaddr_in hostAddr;
	hostAddr.sin_family = AF_INET;
	hostAddr.sin_addr.s_addr = inet_addr(remoteIP);
	hostAddr.sin_port = htons(HOST_PORT);
	bzero(hostAddr.sin_zero, 8);

	int createPidCount = 0;
	for (int i = 0; i < clientCount; i++)
	{
		//创建客户端子进程
		createPidCount++;

		pid_t pid = fork();
		if (pid == 0)
		{
			//连接服务器，开始交互数据
			int clientfd = socket(PF_INET, SOCK_STREAM, 0);
			if (clientfd == -1)
			{
				perror("client-socket");
				_exit(EXIT_FAILURE);
			}

			int result = connect(clientfd, (sockaddr*)&hostAddr, sizeof(sockaddr_in));
			if (result == -1)
			{
				perror("client-socket");
				_exit(EXIT_FAILURE);
			}

			char buff[1024];
			int lenBuff = 1024;

			strcpy(buff, "Hi, I am Mark Chen.");

			//主机序到网络序转换
			uint16_t len = strlen(buff);

			uint16_t nsLen = htons(len);
			lenBuff = send(clientfd, &nsLen, sizeof(uint16_t), MSG_DONTWAIT);
			lenBuff = send(clientfd, buff, len, MSG_DONTWAIT);

			int recvCount = recv(clientfd, buff, lenBuff, MSG_TRUNC);

			close(clientfd);

			_exit(EXIT_SUCCESS);
		}
		else if (pid > 0)
		{
			printf("%06d new client: %d\n", i + 1, pid);

			//及时清理一次已经结束的子进程，为后续的fork调用预留系统资源
			//while (wait(NULL) != -1) {};

			//每次只清理一个子进程,提高父进程执行效率
			//wait(NULL);

			//在创建一定数量的子进程后开始清理子进程
			if (createPidCount >= 100)
			{
				while (wait(NULL) != -1) {};
				createPidCount = 0;
			}
		}
		else
		{
			perror("fork");
		}
	}

	return ret;
}