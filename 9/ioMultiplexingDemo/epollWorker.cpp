#include "epollWorker.h"
#include <memory.h>

#include <sys/epoll.h>
#include <stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

EpollWorker* EpollWorker::getInstance()
{
	static EpollWorker worker;

	return &worker;
}

EpollWorker::EpollWorker()
	: baseWorker()
	, m_askForExit(false)
	, m_hasNewClient(false)
{
	pthread_spin_init(&m_spin, PTHREAD_PROCESS_SHARED);

	start();
}

EpollWorker::~EpollWorker()
{
	pthread_spin_destroy(&m_spin);

	askForExit();
	stop();
}

bool EpollWorker::start()
{
	int ret = pthread_create(&m_thr, NULL, EpollWorker::run, this);
	return (ret == 0);
}

void EpollWorker::askForExit()
{
	m_askForExit = true;
}

void EpollWorker::stop()
{
	pthread_join(m_thr, NULL);
}

void EpollWorker::addClient(clientInfo* info)
{
	SpinLocker locker(&m_spin);

	m_addClientList.push_back(info);
	m_hasNewClient = true;
}

void* EpollWorker::run(void* arg)
{
	int result = 0;
	char buff[BUFF_LEN];

	EpollWorker* This = (EpollWorker*)arg;
	assert(This);

	int epfd = epoll_create(EPOLL_SIZE);
	if (epfd == -1)
	{
		perror("epoll_create");
		pthread_exit(NULL);
	}

	while (This->m_askForExit == false)
	{
		//获取新的客户端连接
		std::vector<clientInfo*> newClientList;
		{
			//自旋锁：要求实时性，临界时间很短的情况
			SpinLocker locker(&This->m_spin);

			if (This->m_hasNewClient)
			{
				newClientList = This->m_addClientList;

				This->m_addClientList.clear();
				This->m_hasNewClient = false;
			}
		}

		//添加新的客户端
		if (newClientList.size() > 0)
		{
			for (auto clientItem = newClientList.begin(); clientItem != newClientList.end(); clientItem++)
			{
				clientInfo* pInfo = *clientItem;
				if (!pInfo)
					continue;

				struct epoll_event event;
				event.events = EPOLLIN | EPOLLET;	//加上EPOLLET就是ET模式
				event.data.ptr = pInfo;

				int clientfd = pInfo->fd;
				result = epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &event);
				if (result == -1)
				{
					perror("epoll_ctl_1");
				}
			}
		}

		//处理客户端socket事件
		struct epoll_event events[WAIT_COUNT];
		int fdns = epoll_wait(epfd, events, WAIT_COUNT, 10);
		if (fdns < 0)
		{
			perror("epoll_wait");
		}
		else
		{
			for (int i = 0; i < fdns; i++)
			{
				struct epoll_event* epevent = events + i;

				clientInfo* cinfo = (clientInfo*)epevent->data.ptr;
				if (!cinfo)
					continue;

				int clientfd = cinfo->fd;
				if (clientfd < 0)
					continue;

				if (epevent->events == EPOLLIN)
				{
					//读取长度
					int readLen = 0;
					uint16_t len = 0;

					readLen = recv(clientfd, &len, sizeof(uint16_t), MSG_WAITALL);
					uint16_t lenTmp = ntohs(len);

					//读取消息
					memset(buff, 0x00, BUFF_LEN);
					size_t lenRead = lenTmp;
					readLen = recv(clientfd, buff, lenRead, MSG_WAITALL);
					if (readLen == -1)
					{
						perror("recv");

						//出现错误，关闭连接
						shutdown(clientfd, SHUT_RDWR);
					}
					else
					{
						printf("%s(%d): %s\n", cinfo->addr, cinfo->port, buff);

						//注册新的事件
						struct epoll_event event;
						event.events = EPOLLOUT | EPOLLET;
						event.data.ptr = cinfo;

						result = epoll_ctl(epfd, EPOLL_CTL_MOD, clientfd, &event);
						if (result == -1)
						{
							perror("epoll_ctl_3");
						}
					}
				}
				else if (epevent->events == EPOLLOUT)
				{
					//发送数据
					bzero(buff, 1024);
					strcpy(buff, "Hi, I have received your message.");
					int lenBuff = strlen(buff);

					int lenSend = send(clientfd, buff, lenBuff, MSG_DONTWAIT | MSG_NOSIGNAL);
					if (lenSend == -1)
					{
						perror("send");
					}

					//关闭连接
					shutdown(clientfd, SHUT_RDWR);  //触发EPOLLHUP
				}
				else
				{
					//处理EPOLLERR、EPOLLHUP自动注册的通知，删除fd注册
					result = epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, NULL);
					if (result == -1)
					{
						perror("epoll_ctl_5");
					}

					//关闭socket文件
					close(clientfd);

					//释放客户端信息
					delete cinfo;
					cinfo = NULL;
				}
			}
		}
	}

	//关闭ep描述符
	close(epfd);

	pthread_exit(NULL);
}
