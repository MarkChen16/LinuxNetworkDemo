#include "pollWorker.h"

#include <memory.h>
#include <assert.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/poll.h>

PollWorker* PollWorker::getInstance()
{
	static PollWorker worker;

	return &worker;
}

PollWorker::PollWorker()
	: baseWorker()
	, m_askForExit(false)
	, m_hasNewClient(false)
{
	pthread_spin_init(&m_spin, PTHREAD_PROCESS_SHARED);
	
	start();
}

PollWorker::~PollWorker()
{
	pthread_spin_destroy(&m_spin);

	askForExit();
	stop();
}

bool PollWorker::start()
{
	int result = pthread_create(&m_thr, NULL, PollWorker::run, this);

	return (result == 0);
}

void PollWorker::askForExit()
{
	m_askForExit = true;
}

void PollWorker::stop()
{
	pthread_join(m_thr, NULL);
}

void PollWorker::addClient(clientInfo* info)
{
	SpinLocker locker(&m_spin);

	m_addClientList.push_back(info);
	m_hasNewClient = true;
}

void* PollWorker::run(void* arg)
{
	int result = 0;
	char buff[BUFF_LEN];

	std::map<int, clientInfo*> cinfoMap;
	std::vector<pollfd> fdlst;

	PollWorker* This = (PollWorker*)arg;
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

				//添加客户端信息
				cinfoMap[pInfo->fd] = pInfo;

				//添加侦听列表
				pollfd plfd;
				plfd.fd = pInfo->fd;
				plfd.events = POLLIN;	//注册想要侦听的事件events
				fdlst.push_back(plfd);
			}
		}

		if (cinfoMap.size() <= 0)
		{
			usleep(10);
			continue;
		}

		//处理客户端socket事件
		result = poll(fdlst.data(), fdlst.size(), 10);
		if (result < 0)
		{
			perror("poll");
		}
		else
		{
			std::vector<int> clrfds;

			for (int i = 0;i < fdlst.size(); i++)
			{
				int clientfd = fdlst[i].fd;
				clientInfo* cinfo = cinfoMap[clientfd];

				//判断当前触发的事件revents
				if (fdlst[i].revents & POLLIN)
				{
					//socket读就绪=========================================================
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
						if (cinfo)
						{
							printf("%s(%d): %s\n", cinfo->addr, cinfo->port, buff);
						}

						//修改想要侦听的事件
						fdlst[i].events = POLLOUT;
					}
				}
				else if (fdlst[i].revents & POLLOUT)
				{
					//socket写就绪=========================================================
					//发送数据
					bzero(buff, 1024);
					strcpy(buff, "Hi, I have received your message.");
					int lenBuff = strlen(buff);

					int lenSend = send(clientfd, buff, lenBuff, MSG_DONTWAIT | MSG_NOSIGNAL);
					if (lenSend == -1)
					{
						perror("send");
					}

					clrfds.push_back(clientfd);
				}
				else
				{
					//socket描述符发现错误、连接关闭，或者poll非法操作======================
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

				//释放客户端信息
				if (cinfoMap[clrfd])
				{
					delete cinfoMap[clrfd];
					cinfoMap.erase(clrfd);
				}

				//移除侦听列表
				for (auto item = fdlst.begin(); item != fdlst.end(); item++)
				{
					if ((*item).fd == clrfd)
					{
						fdlst.erase(item);
						break;
					}
				}
			}

			clrfds.clear();
		}
	}

	pthread_exit(NULL);
}
