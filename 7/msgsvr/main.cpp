#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

/*
示例：

相关函数：
socket.h - socket  ioctl  bind  listen  accept  send  recv  shutdown  close
           socket  ioctl  connect  send  recv  shutdown  close

错误显示：
errno.h - errno  perror  strerror

地址转换：
inet_addr  inet_ntoa

修改允许创建用户最大进程数量
/etc/security/limits.d/90-nproc.conf

ulimit -a
ulimit -u 300000

查看性能命令：
free 查看内存
top -H  查看进程CPU和内存占用率
netstat -i  查看网络错误数量
sar -n SOCK 查看网络流量
iostat -x  查看磁盘读写性能

修改iptables防火墙规则
service iptables status
iptables -F INPUT
iptables -F OUTPUT

*/

#define HOST_PORT 1800
#define HOST_LISTEN_COUNT 20

static void signal_handler(int signo)
{
	//处理系统信号
	if (signo == SIGINT || signo == SIGKILL)
	{
		//Ctrl + C退出程序
		printf("server is closing...\n");
		exit(EXIT_SUCCESS);
	}
	else if (signo == SIGCHLD)
	{
		//给已经结束的子进程收尸
		while (wait(NULL) != -1);
	}
}

int main(int argc, char* argv[])
{
	int sockfd = 0;
	int result = 0;

	signal(SIGKILL, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGCHLD, signal_handler);

	//创建socket文件描述符，因特网协议族的TCP协议，第一个特定类型(一般只有一个，所以是0)
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket");
		//也可以使用strerror，输出错误信息 printf("%s: %s\n", "socket", strerror(errno));

		exit(EXIT_FAILURE);
	}
	printf("init socket ...\n");

	//获取第一块网卡配置的IP地址
	struct ifreq ifr;
	strcpy(ifr.ifr_name, "eth1");

	result = ioctl(sockfd, SIOCGIFADDR, &ifr);
	if (result == -1)
	{
		perror("ioctl");
		exit(EXIT_FAILURE);
	}

	const char* localhostIP = inet_ntoa(((sockaddr_in*)&ifr.ifr_addr)->sin_addr);

	//绑定地址和端口
	struct sockaddr_in hostAddr;
	hostAddr.sin_family = AF_INET;
	hostAddr.sin_port = htons(HOST_PORT);
	hostAddr.sin_addr.s_addr = inet_addr(localhostIP);
	bzero(hostAddr.sin_zero, 8);

	result = bind(sockfd, (sockaddr*)&hostAddr, sizeof(sockaddr_in));
	if (result == -1)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}
	printf("bind socket ...\n");

	//开始侦听
	result = listen(sockfd, HOST_LISTEN_COUNT);
	if (result == -1)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	printf("listen socket ...\n");

	//开始接受客户端连接
	while (true)
	{
		struct sockaddr_in clientAddr;
		socklen_t lenAddr = sizeof(sockaddr_in);

		try
		{
			int clientfd = accept(sockfd, (sockaddr*)&clientAddr, &lenAddr);
			if (clientfd == -1)
			{
				perror("accept");
			}
			else
			{
				//进程分叉
				pid_t pid = fork();
				if (pid == 0)
				{
					close(sockfd);	//关闭父进程的文件描述符，避免误操作

					char buff[1024];
					int lenBuff = 1024;
					bzero(buff, 1024);

					int readLen = recv(clientfd, buff, lenBuff, 0);
					if (readLen > 0)
					{
						printf("%s(%d): %s\n", inet_ntoa(clientAddr.sin_addr), clientAddr.sin_port, buff);

						bzero(buff, 1024);
						strcpy(buff, "Hi, I have received your message.");
						lenBuff = strlen(buff);

						int lenSend = send(clientfd, buff, lenBuff, 0);
					}
					else
					{
						perror("recv");
					}

					close(clientfd);

					//fork子进程用_exit()退出程序，不做清理IO缓冲
					_exit(EXIT_SUCCESS);
				}
				else if (pid > 0)
				{
					close(clientfd);  //关闭子进程的文件描述符，避免误操作

					printf("accept new client ...\n");
				}
				else
				{
					perror("fork");
				}
			}
		}
		catch (...)
		{
		}
	}

	//关闭socket
	close(sockfd);
	//shutdown(sockfd, SHUT_RDWR);	//关闭读和写端，等同于close(sockfd)

	exit(EXIT_SUCCESS);
}
