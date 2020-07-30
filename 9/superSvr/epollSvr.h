#pragma once

#include <pthread.h>
#include <list>
#include <atomic>

#define HOST_LISTEN_MAX 1000000
#define EPOLL_SIZE_MAX	1000000
#define EPOLL_WAIT_MAX	1000

#define BUFF_LEN 1024

class ClientQuery
{
public:
	explicit ClientQuery(int clientfd, const char* addr, int port);
	virtual ~ClientQuery();

	void init(int epfd);

	void doQuery(void* arg);
	void doQueryResponse(void* arg);

private:
	int m_epfd;

	int m_clientfd;
	char m_addr[100];
	int m_port;
};

class EpollSvr
{
public:
	explicit EpollSvr();
	virtual ~EpollSvr();

	bool start(const char* addr, int port);
	void stop();

protected:
	void addClientQuery(ClientQuery* query);

protected:
	static void* run_listen(void* arg);
	static void* run_epoll(void* arg);

private:
	std::atomic_bool m_askForExit;
	pthread_t m_listenThr;
	pthread_t m_epollThr;
	bool m_running;

	pthread_spinlock_t m_spin;
	std::list<ClientQuery*> m_newClientList;

	char m_addr[100];
	int m_port;
};

class SpinLocker
{
public:
	explicit SpinLocker(pthread_spinlock_t* spin);
	virtual ~SpinLocker();

protected:
	pthread_spinlock_t* m_spin;

};
