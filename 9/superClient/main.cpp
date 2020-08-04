#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/epoll.h>
#include <signal.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <map>


/*
使用epoll IO模型模拟N个客户端，对服务端进程压力测试

*/

#define HOST_PORT 1800
#define EPOLL_CREATE_SIZE 70000
#define EPOLL_WAIT_SIZE 1000

struct QueryInfo
{
	int sockfd;
	int epfd;
	int status;

	void doWrite()
	{
		char buff[1024];
		int lenBuff = 1024;

		strcpy(buff + sizeof(uint16_t), "Hi, Mark.");

		//主机序到网络序转换
		uint16_t len = strlen(buff + sizeof(uint16_t));
		uint16_t nsLen = htons(len);
		memcpy(buff, &nsLen, sizeof(uint16_t));

		//注意：如果长度和数据分开发送，有可能数据报文先到达，读取顺序就错了
		lenBuff = send(sockfd, buff, len + sizeof(uint16_t), 0);
		if (lenBuff == -1)
		{
			perror("send");

			//发送失败，关闭连接
			shutdown(sockfd, SHUT_RDWR);
		}
		else
		{
			//注册读就绪事件
			epoll_event readEvent;
			readEvent.events = EPOLLIN | EPOLLET;
			readEvent.data.ptr = this;

			int result = epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &readEvent);
			if (result == -1)
			{
				perror("epoll_ctl");
			}
		}
	}

	void doRead()
	{
		char buff[1024];
		int lenBuff = 1024;

		int recvCount = recv(sockfd, buff, lenBuff, MSG_TRUNC);
		if (recvCount == -1)
		{
			perror("recv");
		}

		//关闭连接
		shutdown(sockfd, SHUT_RDWR);
	}
};

bool user_canel = false;
void sigint_handler(int signo)
{
	user_canel = true;
}

int main(int argc, char* argv[])
{
	int ret = 0;
	int result = 0;

	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, SIG_IGN);

	std::map<int, QueryInfo*> queryMap;

	int clientCount = 60000;
	const char* remoteIP = "192.168.189.134";
	
	if (argc >= 2)
		remoteIP = argv[1];

	if (argc >= 3)
		clientCount = atoi(argv[2]);

	//创建epoll描述符
	int epfd = epoll_create(EPOLL_CREATE_SIZE);
	if (epfd == -1)
	{
		perror("epoll_create");
	}

	//创建N个客户端
	sockaddr_in hostAddr;
	hostAddr.sin_family = AF_INET;
	hostAddr.sin_addr.s_addr = inet_addr(remoteIP);
	hostAddr.sin_port = htons(HOST_PORT);
	bzero(hostAddr.sin_zero, 8);

	int i = 0;
	for (i = 0; i < clientCount; i++)
	{
		if (user_canel)
			break;

		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == -1)
		{
			perror("socket");
			break;
		}

		result = connect(sockfd, (sockaddr*)&hostAddr, sizeof(sockaddr_in));
		if (result == -1)
		{
			perror("connect");
			break;
		}
		else
		{
			//注册写就绪事件
			QueryInfo* newQuery = new QueryInfo();
			newQuery->epfd = epfd;
			newQuery->sockfd = sockfd;
			newQuery->status = 0;
			queryMap[sockfd] = newQuery;

			epoll_event writeEvent;
			writeEvent.events = EPOLLOUT | EPOLLET;
			writeEvent.data.ptr = newQuery;
			result = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &writeEvent);
			if (result == -1)
			{
				perror("epoll_ctl");
			}

			printf("new connection(%d)\n", sockfd);
		}
	}

	printf("init connections = %d\n", i);

	//处理epoll事件
	epoll_event eventList[EPOLL_WAIT_SIZE];
	int fdns = 0;

	while (!user_canel)
	{
		fdns = epoll_wait(epfd, eventList, EPOLL_WAIT_SIZE, 10);
		if (fdns > 0)
		{
			for (int i = 0; i < fdns; i++)
			{
				epoll_event* currEvent = eventList + i;
				QueryInfo* currQuery = (QueryInfo*)currEvent->data.ptr;
				if (currEvent->events & EPOLLOUT)
				{
					currQuery->doWrite();
				}
				else if (currEvent->events & EPOLLIN)
				{
					currQuery->doRead();
				}
				else
				{
					//删除内核fd结构
					result = epoll_ctl(epfd, EPOLL_CTL_DEL, currQuery->sockfd, NULL);
					if (result == -1)
					{
						perror("epoll_ctl");
					}

					//关闭描述符
					close(currQuery->sockfd);

					//删除
					queryMap.erase(currQuery->sockfd);

					delete currQuery;
					currQuery = NULL;
				}
			}
		}
	}

	//退出前关闭剩余连接
	for (auto item = queryMap.begin(); item != queryMap.end(); item++)
	{
		QueryInfo* queryItem = item->second;

		close(queryItem->sockfd);

		delete queryItem;
	}
	queryMap.clear();

	//关闭epoll描述符
	close(epfd);

	printf("\nTest is Done.\n");
	return ret;
}
