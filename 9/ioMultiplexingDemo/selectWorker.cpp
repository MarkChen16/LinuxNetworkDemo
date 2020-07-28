#include "selectWorker.h"

#include <memory.h>
#include <assert.h>

#include <sys/epoll.h>
#include <stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>

#include <sys/select.h>

SelectWorker* SelectWorker::getInstance()
{
	static SelectWorker worker;

	return &worker;
}

SelectWorker::SelectWorker()
	: baseWorker()
	, m_askForExit(false)
	, m_hasNewClient(false)
{
	pthread_spin_init(&m_spin, PTHREAD_PROCESS_SHARED);

	start();
}

SelectWorker::~SelectWorker()
{
	pthread_spin_destroy(&m_spin);

	askForExit();
	stop();
}

bool SelectWorker::start()
{
	int result = pthread_create(&m_thr, NULL, SelectWorker::run, this);

	return (result == 0);
}

void SelectWorker::askForExit()
{
	m_askForExit = true;
}

void SelectWorker::stop()
{
	pthread_join(m_thr, NULL);
}

void SelectWorker::addClient(clientInfo* info)
{
	SpinLocker locker(&m_spin);

	m_addClientList.push_back(info);
	m_hasNewClient = true;
}

void* SelectWorker::run(void* arg)
{
	int result = 0;
	char buff[BUFF_LEN];

	std::vector<clientInfo*> cinfoList;
	fd_set readfds, writefds;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	SelectWorker* This = (SelectWorker*)arg;
	assert(This);

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

				int clientfd = pInfo->fd;
				FD_SET(clientfd, &readfds);

				cinfoList.push_back(pInfo);
			}
		}

		if (cinfoList.size() <= 0)
		{
			usleep(10);
			continue;
		}

		//处理客户端socket事件
		int nfds = getMaxFD(cinfoList);
		fd_set rfds = readfds;
		fd_set wfds = writefds;
		timeval timeout;
		timeout.tv_usec = 10000;
		result = select(nfds + 1, &rfds, &wfds, NULL, &timeout);
		if (result < 0)
		{
			perror("select");
		}
		else
		{
			std::vector<int> clrfds;

			for (auto item = cinfoList.begin(); item != cinfoList.end(); item++)
			{
				clientInfo* cinfo = *item;
				if (!cinfo)
					continue;

				int clientfd = cinfo->fd;
				if (FD_ISSET(clientfd, &rfds))
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
						clrfds.push_back(clientfd);
					}
					else
					{
						printf("%s(%d): %s\n", cinfo->addr, cinfo->port, buff);

						//加入侦听写的FD列表，清除读的FD列表
						FD_CLR(clientfd, &readfds);
						FD_SET(clientfd, &writefds);
					}
				}
				else if (FD_ISSET(clientfd, &wfds))
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

					//通信结束
					FD_CLR(clientfd, &writefds);

					clrfds.push_back(clientfd);
				}
			}

			//关闭要清除的FD
			for (auto clrItem = clrfds.begin(); clrItem != clrfds.end(); clrItem++)
			{
				int clrfd = *clrItem;

				//关闭连接
				shutdown(clrfd, SHUT_RDWR);

				//关闭socket描述符
				close(clrfd);

				//移除记录
				for (auto item = cinfoList.begin();item != cinfoList.end();item++)
				{
					if ((*item)->fd == clrfd)
					{
						//释放客户端信息
						delete (*item);

						cinfoList.erase(item);
						break;
					}
				}
			}

			clrfds.clear();
		}
	}

	pthread_exit(NULL);
}

int SelectWorker::getMaxFD(std::vector<clientInfo*>& cinfoList)
{
	int maxFD = -1;

	for (auto item = cinfoList.begin(); item != cinfoList.end(); item++)
	{
		if (maxFD < (*item)->fd)
			maxFD = (*item)->fd;
	}

	return maxFD;
}
