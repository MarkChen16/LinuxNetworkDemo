#include <stdio.h>
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

static void signal_handler(int signo)
{
	if (signo == SIGINT)
	{
		exit(EXIT_SUCCESS);
	}
	else
	{
		//给结束的子进程收尸
		while (wait(NULL) != -1);
	}
}

int main(int argc, char* argv[])
{
	int result = 0;
	int clientCount = 100000;
	const char* remoteIP = "127.0.0.1";

	if (argc >= 2)
		remoteIP = argv[1];

	if (argc >= 3)
		clientCount = atoi(argv[2]);

	signal(SIGINT, signal_handler);
	signal(SIGCHLD, signal_handler);

	sockaddr_in hostAddr;
	hostAddr.sin_family = AF_INET;
	hostAddr.sin_addr.s_addr = inet_addr(remoteIP);
	hostAddr.sin_port = htons(HOST_PORT);
	bzero(hostAddr.sin_zero, 8);

	//创建客户端子进程
	for (int i = 0; i < clientCount; i++)
	{
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

			result = connect(clientfd, (sockaddr*)&hostAddr, sizeof(sockaddr_in));
			if (result == -1)
			{
				perror("client-socket");
				_exit(EXIT_FAILURE);
			}

			char buff[1024];
			int lenBuff = 1024;

			strcpy(buff, "Hi, I am Mark Chen.");
			lenBuff = send(clientfd, buff, strlen(buff), 0);

			int recvCount = recv(clientfd, buff, lenBuff, 0);

			close(clientfd);

			_exit(EXIT_SUCCESS);
		}
		else if (pid > 0)
		{
			printf("%05d new client: %d\n", i, pid);
		}
		else
		{
			perror("fork");
		}
	}

	pause();
	exit(EXIT_SUCCESS);
}
