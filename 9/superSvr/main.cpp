#include <stdio.h>
#include <unistd.h>

#include "threadpool.h"
#include "epollSvr.h"

/*
例子：采用与Nginx一样的epoll and kqueue模型


高并发服务：epoll通过事件通知解决IO阻塞问题，多线程解决并发通信问题，线程池解决多线程频率创建销毁带来效率问题；

相关技术：IO复用、多线程、线程池、数据库连接池，多进程、redis缓存中间件

*/

int main(int argc, char* argv[])
{
	int ret = 0;

	EpollSvr superSvr;
	superSvr.start("192.168.189.134", 1800);

	pause();
	return ret;
}
