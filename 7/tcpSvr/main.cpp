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
后端服务程序的多进程和多线程的选择，或者两者都用。

两者特点：
1、数据共享
进程之间需要IPC，但数据私有，同步简单；
线程之间共享同一份进程数据，但同步复杂，需要互斥、读写锁和信号量各种同步机制；

2、内存和CPU使用率
进程占用内存多，CPU使用率低；
线程占有内存少，CPU使用率多，需要在线程之间切换；

3、创建销毁开销
进程开销大，切换速度慢；
线程开销小，切换速度快；

4、可靠性
进程间独立运行，可靠性高；
线程在一个进程内运行，一个异常导致整个进程退出，可靠性低；

5、分布性
进程适应于多核，多机分布；
线程适应于多核分布；

两者怎么选：
1、处理简单，需要频繁创建销毁，优先用线程；

2、需要大量计算，优先用线程；

3、数据强相关的处理，优先使用线程；

4、多机分布的用进程；多核分布的用线程；

5、可以采用多进程，子进程采用多线程的方式，高效又可靠；


相关函数：
socket.h
服务端：socket  ioctl  bind  listen  accept  send  recv  shutdown  close
客户端：socket  ioctl  connect  send  recv  shutdown  close

文件描述符操作：
write  read  发送或接收一个缓冲区数据
writev  readv  发送或接收多个缓冲区数据

TCP发送接收数据：
send  recv  发送或接收一个缓冲区数据

UDP发送接收报文：
writeto  readfrom  发送或接收一个报文
sendmsg  recvmsg  发送或接收多个报文

错误显示：
errno.h - errno  perror  strerror

地址转换：
inet_addr  inet_ntoa  inet_aton

字节序转换：
arpa/inet.h - htonl  htons   主机序到网络序转换，后面l和s表示类型，分别表示16位还是32位
			  ntohl  ntohs   网络序到主机序转换

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

防火墙规则
ufw enable
ufw disable

/etc/sysconfig/iptables

清除所有规则
iptables -L
iptables -F INPUT
iptables -F OUTPUT
iptables -F FORWARD
iptables -F

//添加规则
iptables -A -s 192.168.189.137 -p tcp -dport 1800 -j ACCEPT

service iptables status
service iptables save
service iptables restart

Ctrl + Z暂时运行当前进程，fg [n]将进程放在前台继续运行，bg [n]将进程放在后台继续运行； 

计算某个进程的父子进程数量
ps -el | grep tcpClient.out | wc -c

杀死所有进程
killall -9 tcpClient.out

*/

#define HOST_PORT 1800
#define HOST_LISTEN_COUNT 20000

static void sig_int(int signo)
{
	printf("server is closing...\n");
	exit(EXIT_SUCCESS);
}

void sig_chil(int sigNo)
{
	while (wait(NULL) != -1) {};

	//防止信号被重置
	signal(SIGCHLD, sig_chil);
}

void sig_pipe(int sigNo)
{
	signal(SIGPIPE, sig_pipe);
}

int main(int argc, char* argv[])
{
	int sockfd = 0;
	int ret = 0;
	int result = 0;

	signal(SIGINT, sig_int);
	signal(SIGCHLD, sig_chil);
	signal(SIGPIPE, sig_pipe);

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
	strcpy(ifr.ifr_name, "eth0");

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
	printf("bind socket(%s: %i)\n", localhostIP, HOST_PORT);

	//开始侦听，队列最多包含N个连接，如果队列已满，协议支持重传，连接会被忽略稍后再尝试连接；否则直接返回拒绝给客户端；
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

					int readLen = 0;
					uint16_t len = 0;
					
					readLen = recv(clientfd, &len, sizeof(uint16_t), MSG_WAITALL);
					len = ntohs(len);

					readLen = recv(clientfd, buff, len, MSG_WAITALL);
					if (readLen > 0)
					{
						printf("%s(%d)(%d): %s\n", inet_ntoa(clientAddr.sin_addr), clientAddr.sin_port, len, buff);

						bzero(buff, 1024);
						strcpy(buff, "Hi, I have received your message.");
						lenBuff = strlen(buff);

						int lenSend = send(clientfd, buff, lenBuff, MSG_DONTWAIT);
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

	return ret;
}
