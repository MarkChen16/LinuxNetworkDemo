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

int main(int argc, char* argv[])
{
	int ret = 0;
	int clientCount = 100000;
	const char* remoteIP = "192.168.189.134";

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

	for (int i = 0; i < clientCount; i++)
	{
		//创建客户端子进程
		pid_t ppid = fork(); //调用两次fork，让孙进程成为遗孤，托管到init进程
		if (ppid == 0)
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

				int result = connect(clientfd, (sockaddr*)&hostAddr, sizeof(sockaddr_in));
				if (result == -1)
				{
					perror("client-socket");
					_exit(EXIT_FAILURE);
				}

				char buff[1024];
				int lenBuff = 1024;

				strcpy(buff + sizeof(uint16_t), "Hi, Mark.");

				//主机序到网络序转换
				uint16_t len = strlen(buff + sizeof(uint16_t));
				uint16_t nsLen = htons(len);
				memcpy(buff, &nsLen, sizeof(uint16_t));

				//注意：如果长度和数据分开发送，有可能数据报文先到达，读取顺序就错了
				lenBuff = send(clientfd, buff, len + sizeof(uint16_t), 0);

				int recvCount = recv(clientfd, buff, lenBuff, MSG_TRUNC);

				close(clientfd);

				_exit(EXIT_SUCCESS);
			}
			else if (pid > 0)
			{
				printf("%07d new client: %d\n", i + 1, pid);

				_exit(EXIT_SUCCESS);
			}
			else
			{
				perror("fork");
			}
		}
		else if (ppid > 0)
		{
			waitpid(ppid, NULL, 0);
		}
		else
		{
			i--;

			usleep(500 * 1000);
		}
	}

	return ret;
}
