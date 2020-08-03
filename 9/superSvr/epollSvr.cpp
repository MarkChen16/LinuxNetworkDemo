#include "epollSvr.h"

#include <stdio.h>
#include <string.h>
#include <list>
#include <assert.h>

#include <sys/types.h>
#include <sys/times.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <sys/epoll.h>

#include "threadpool.h"


ClientQuery::ClientQuery(int clientfd, const char* addr, int port)
	: m_clientfd(clientfd)
	, m_port(port)
{
	strcpy(m_addr, addr);
}

ClientQuery::~ClientQuery()
{
}

void ClientQuery::init(int epfd)
{
	m_epfd = epfd;

	//注册读就绪事件
	struct epoll_event readEvent;
	readEvent.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	readEvent.data.ptr = this;

	int result = epoll_ctl(epfd, EPOLL_CTL_ADD, m_clientfd, &readEvent);
	if (result == -1)
	{
		perror("ClientQuery::init()::epoll_ctl()");
	}
}

void ClientQuery::doQuery(void* arg)
{
	int result = 0;
	int epfd = m_epfd, clientfd = m_clientfd;

	//读取长度
	int readLen = 0;
	uint16_t len = 0;

	readLen = recv(clientfd, &len, sizeof(uint16_t), MSG_WAITALL);
	uint16_t lenTmp = ntohs(len);

	//读取消息
	char buff[BUFF_LEN];
	bzero(buff, BUFF_LEN);

	size_t lenRead = lenTmp;
	readLen = recv(clientfd, buff, lenRead, MSG_WAITALL);
	if (readLen == -1)
	{
		perror("ClientQuery::doConnect()::recv()");

		//出现错误，关闭连接
		shutdown(clientfd, SHUT_RDWR);
	}
	else
	{
		printf("%s(%d): %s\n", m_addr, m_port, buff);

		//模拟处理请求时间
		usleep(50 * 1000);

		//注册新的事件
		struct epoll_event event;
		event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
		event.data.ptr = this;

		result = epoll_ctl(epfd, EPOLL_CTL_MOD, clientfd, &event);
		if (result == -1)
		{
			perror("ClientQuery::doConnect()::epoll_ctl()");
		}
	}
}

void ClientQuery::doQueryResponse(void* arg)
{
	int result = 0;
	int epfd = m_epfd, clientfd = m_clientfd;

	char buff[BUFF_LEN];
	bzero(buff, BUFF_LEN);

	strcpy(buff, "Hi, I have received your message.");
	int lenBuff = strlen(buff);

	int lenSend = send(clientfd, buff, lenBuff, MSG_DONTWAIT | MSG_NOSIGNAL);
	if (lenSend == -1)
	{
		perror("ClientQuery::doQueryResponse()::send()");
	}

	//处理EPOLLERR、EPOLLHUP自动注册的通知，删除fd注册
	result = epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, NULL);
	if (result == -1)
	{
		perror("ClientQuery::doClose()::epoll_ctl()");
	}

	//关闭连接
	shutdown(clientfd, SHUT_RDWR);  //触发EPOLLHUP

	//关闭socket文件
	close(clientfd);
}

EpollSvr::EpollSvr()
	: m_askForExit(false)
	, m_running(false)
{
	strcpy(m_addr, "localhost");
	m_port = 1800;

	pthread_spin_init(&m_spin, PTHREAD_PROCESS_SHARED);
}

EpollSvr::~EpollSvr()
{
	stop();

	pthread_spin_destroy(&m_spin);
}

bool EpollSvr::start(const char* addr, int port)
{
	if (m_running)
		return true;

	int result = 0;
	strcpy(m_addr, addr);
	m_port = port;

	result = pthread_create(&m_epollThr, NULL, EpollSvr::run_epoll, this);
	if (result == -1)
	{
		perror("EpollSvr::start()1");
		return false;
	}

	result = pthread_create(&m_listenThr, NULL, EpollSvr::run_listen, this);
	if (result == -1)
	{
		perror("EpollSvr::start()2");
		return false;
	}

	m_running = true;
	return true;
}

void EpollSvr::stop()
{
	if (!m_running)
		return;

	int result = 0;
	m_askForExit = true;

	result = pthread_join(m_listenThr, NULL);
	if (result == -1)
	{
		perror("EpollSvr::stop()1");
	}

	result = pthread_join(m_epollThr, NULL);
	if (result == -1)
	{
		perror("EpollSvr::stop()2");
	}

	m_running = false;
}

void EpollSvr::addClientQuery(ClientQuery* query)
{
	SpinLocker locker(&m_spin);

	m_newClientList.push_back(query);
}

void* EpollSvr::run_listen(void* arg)
{
	//获取本实例
	EpollSvr* This = (EpollSvr*)arg;
	assert(This);

	int result = 0;
	int hostfd = 0, clientfd = 0;
	std::list<ClientQuery*> queryList;

	//创建服务端socket，指定为以太网的tcp协议
	hostfd = socket(AF_INET, SOCK_STREAM, 0);
	if (hostfd == -1)
	{
		perror("EpollSvr::run()::socket()");
	}
	else
	{
		//绑定端口
		struct sockaddr_in hostAddr;
		hostAddr.sin_family = AF_INET;
		hostAddr.sin_addr.s_addr = inet_addr(This->m_addr);
		hostAddr.sin_port = htons(This->m_port);
		bzero(hostAddr.sin_zero, 8);

		result = bind(hostfd, (sockaddr*)&hostAddr, sizeof(sockaddr_in));
		if (result == -1)
		{
			perror("EpollSvr::run()::bind()");
		}
		else
		{
			//开始侦听
			result = listen(hostfd, HOST_LISTEN_MAX);
			if (result == -1)
			{
				perror("EpollSvr::run()::listen()");
			}
			else
			{
				printf("(%s: %d)Service start ...\n", This->m_addr, This->m_port);

				//开始接受客户端连接
				while (!This->m_askForExit)
				{
					try
					{
						struct sockaddr_in clientAddr;
						socklen_t lenAddr = sizeof(sockaddr_in);

						int clientfd = accept(hostfd, (sockaddr*)&clientAddr, &lenAddr);
						if (clientfd == -1)
						{
							perror("accept");
						}
						else
						{
							ClientQuery* newQuery = new ClientQuery(clientfd, inet_ntoa(clientAddr.sin_addr), clientAddr.sin_port);
							This->addClientQuery(newQuery);
						}
					}
					catch (...)
					{
						perror("catch");
					}
				}

				//关闭侦听
				shutdown(hostfd, SHUT_RDWR);
			}
		}

		//关闭服务端描述符
		close(hostfd);
		printf("Service stop.\n");
	}

	pthread_exit(NULL);
}

void* EpollSvr::run_epoll(void* arg)
{
	int result = 0;

	//获取本实例
	EpollSvr* This = (EpollSvr*)arg;
	assert(This);

	//客户端请求数据
	std::list<ClientQuery*> queryList;

	//创建ep描述符
	int epfd = epoll_create(EPOLL_SIZE_MAX);
	if (epfd == -1)
	{
		perror("EpollSvr::run()::epoll_create()");
	}
	else
	{
		//开始处理客户端连接通信
		ThreadPool pool;
		struct epoll_event waitEvents[EPOLL_WAIT_MAX];

		while (This->m_askForExit == false)
		{
			//处理新的客户端连接
			std::list<ClientQuery*> newClientList;
			{
				SpinLocker locker(&This->m_spin);

				if (!This->m_newClientList.empty())
				{
					newClientList = This->m_newClientList;
					This->m_newClientList.clear();
				}
			}

			for (auto item = newClientList.begin(); item != newClientList.end();item++)
			{
				ClientQuery* newQuery = (ClientQuery*)(*item);
				newQuery->init(epfd);

				queryList.push_back(newQuery);
			}

			//开始侦听事件
			int fdns = epoll_wait(epfd, waitEvents, EPOLL_WAIT_MAX, 10);
			if (fdns > 0)
			{
				for (int i = 0; i < fdns; i++)
				{
					struct epoll_event* currEvent = waitEvents + i;
					ClientQuery* currQuery = (ClientQuery*)currEvent->data.ptr;

					if (currEvent->events & EPOLLIN)
					{
						//socket描述符读就绪事件
						std::function<void(void*)> fun = std::bind(&ClientQuery::doQuery, currQuery, std::placeholders::_1);
						pool.addTask(fun);
					}
					else if (currEvent->events & EPOLLOUT)
					{
						//socket描述符写就绪事件
						std::function<void(void*)> fun = std::bind(&ClientQuery::doQueryResponse, currQuery, std::placeholders::_1);
						pool.addTask(fun);
					}
					else
					{
						printf("othre event.\n");
					}
				}
			}
		}

		//关闭ep描述符
		close(epfd);
	}

	//清除客户端请求数据
	for (auto item = queryList.begin(); item != queryList.end(); item++)
	{
		delete* item;
	}
	queryList.clear();
}

SpinLocker::SpinLocker(pthread_spinlock_t* spin)
	: m_spin(spin)
{
	pthread_spin_lock(m_spin);
}

SpinLocker::~SpinLocker()
{
	pthread_spin_unlock(m_spin);
}
