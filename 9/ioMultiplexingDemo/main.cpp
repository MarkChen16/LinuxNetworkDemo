#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "baseWorker.h"
#include "epollWorker.h"
#include "selectWorker.h"

/*

select IO复用：
适用于描述符量级小的场景，通过轮询三种fd列表的方式通知，有数量限制，一般是1024；

poll IO复用：
适用于描述符量级小的场景，优化select，侦听每个fd的某个事件；

epoll IO复用：
适用于描述符量级比较大的场景，通过注册fd和回调函数，内部使用红黑树查询；

例子：主线程用于处理客户端连接，工作线程负责通信；


修复单个进程打开的最大文件数量：getrlimit  getrlimit

*/

#define HOST_PORT 1800
#define HOST_LISTEN_COUNT 20000

void sigint_handler(int signo)
{
	printf("System is shutdowning ...\n");

	exit(EXIT_SUCCESS);
}

void sigpipe_handler(int signo)
{
	printf("catch pipe error!\n");

	signal(SIGPIPE, sigpipe_handler);
}

int main(int argc, char* argv[])
{
	int ret = 0;

	int sockfd = 0;
	int result = 0;

	int ioMode = 1;
	if (argc >= 2)
	{
		ioMode = atoi(argv[1]);
	}

	baseWorker* worker = SLWORKER;
	if (ioMode == 1)
		worker = SLWORKER;
	else if (ioMode == 2)
		worker = EPWORKER;

	//处理信号
	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, sigpipe_handler);

	//修改单进程打开的最大文件数量
	struct rlimit limitData;
	getrlimit(RLIMIT_NOFILE, &limitData);
	
	limitData.rlim_cur = 100000;
	limitData.rlim_max = 1000000;
	setrlimit(RLIMIT_NOFILE, &limitData);

	//创建服务端socket，指定是因特网的TCP协议
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	printf("init socket ...\n");

	//获取第一块网卡配置的IP地址
	struct ifreq ifr;
	strcpy(ifr.ifr_name, "eth0");

	result = ioctl(sockfd, SIOCGIFADDR, &ifr);
	if (result == -1)
	{
		perror("ioctl");
		exit(EXIT_FAILURE);
	}

	const char* localhostIP = inet_ntoa(((sockaddr_in*)&ifr.ifr_addr)->sin_addr);
	printf("localhost = %s\n", localhostIP);

	//绑定地址和端口
	struct sockaddr_in hostAddr;
	hostAddr.sin_family = AF_INET;
	hostAddr.sin_port = htons(HOST_PORT);
	hostAddr.sin_addr.s_addr = inet_addr(localhostIP);
	bzero(hostAddr.sin_zero, 8);

	result = bind(sockfd, (sockaddr*)&hostAddr, sizeof(sockaddr_in));
	if (result == 1)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}
	printf("bind socket(%s: %i)\n", localhostIP, HOST_PORT);

	//开始侦听
	result = listen(sockfd, HOST_LISTEN_COUNT);
	if (result == -1)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	printf("listen socket(max = %i) ...\n", HOST_LISTEN_COUNT);

	//开始接受客户端连接
	while (true)
	{
		try
		{
			struct sockaddr_in clientAddr;
			socklen_t lenAddr = sizeof(sockaddr_in);

			int clientfd = accept(sockfd, (sockaddr*)&clientAddr, &lenAddr);
			if (clientfd == -1)
			{
				perror("accept");
			}
			else
			{
				clientInfo* info = new clientInfo();
				info->fd = clientfd;
				strcpy(info->addr, inet_ntoa(clientAddr.sin_addr));
				info->port = clientAddr.sin_port;

				worker->addClient(info);
			}
		}
		catch (...)
		{
			perror("catch");
		}
	}

	//关闭socket
	shutdown(sockfd, SHUT_RDWR);

	return ret;
}
