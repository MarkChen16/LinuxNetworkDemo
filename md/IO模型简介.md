## 前言
每个连接分配一个子进程或者线程去处理太low，调度器切换效率太低，也限制了客户端最大连接数；

### 阻塞模型
socket默认是阻塞连接、读写操作；
* 连接操作：有客户端连接进来，正在返回；没有连接就阻塞等待；
* 读操作：缓冲区空，读等待；
* 写操作：缓冲区满，写等待；

### 非阻塞模型
设置noblocking的话，就是非阻塞操作；不断去检查是否读就绪或者写就绪，CPU时间消耗非常大；

### IO复用
* select IO模型：传入3种FD列表，通过轮询状态，返回就绪的FD列表；每个列表最多只能处理1024个FD；
* poll IO模型：传入【FD、侦听事件】列表，通过轮询状态，返回【FD、侦听事件、触发事件】；没有FD的限制，但FD数量增大时，每次调用时拷贝数据结构和轮询状态效率比较低；

### 信号驱动
* epoll IO模型：用户程序通过内核注册FD的IO事件，socket的IO事件发生后，内核通过高速缓存的红黑树查找对应的FD设置触发的事件，在用户程序epoll_wait后一并返回；没有FD限制，减少每次数据结构拷贝；
	1. LT水平触发：只要FD还有数据未读就触发事件，注册的事件永久有效；
	2. ET边缘触发：FD的事件在一段时间内只触发一次，注册的事件只能触发一次，再次侦听事件需要修改注册事件；
	
	ps: select属于POSIX，epoll只有linux才有；
	
* kqueue IO模型：kqueue 提供 kqueue()、kevent() 两个系统调用、 struct kevent 结构和EV_SET()宏函数，让用户程序高效处理IO复用。
	1. int kqueue(void); 生成一个内核事件队列; 
	2. int kevent(int kq, const struct kevent *changelist, int nchanges, struct kevent *eventlist, int nevents, const struct timespec *timeout);


### 完全异步IO
只有windows下的IO完成端口是完全异步，相比epoll IO模型，在内核空间复制数据到用户空间都是异步; 其他IO模型都会有一点阻塞操作；

## 系统配置
支持高并发，保持100W以上的连接，需要修改系统相关配置；

### 最大同时打开文件的数量
* 修改最大同时打开文件的数量 /etc/security/limits.conf
	```
	* soft nofile 1048756
	* hard nofile 1048756
	```

* 修改系统级最大同时打开文件的数量
	```
	cat /proc/sys/fs/file-max
	12158
	
	/etc/rc.local
	echo 1048756 > /proc/sys/fs/file-max
	```

* 应用新的配置
	```
	sysctl -p /etc/sysctl.conf
	```

### 服务端修改配置
并发数量跟内存有很大关系；

	#设置最大的监听队列的长度, 默认值为128
	net.core.somaxconn = 10000

	#系统读写缓存，默认值
	net.core.rmem_default = 262144
	net.core.wmem_default = 262144

	#系统读写缓存
	net.core.rmem_max = 16777216
	net.core.wmem_max = 16777216
	net.ipv4.tcp_rmem = 4096 4096 16777216
	net.ipv4.tcp_wmem = 4096 4096 16777216
	net.ipv4.tcp_mem = 786432 3145728  4194304
	net.ipv4.tcp_max_syn_backlog = 16384
	net.core.netdev_max_backlog = 20000
	net.ipv4.tcp_fin_timeout = 15
	net.ipv4.tcp_max_syn_backlog = 16384
	net.ipv4.tcp_tw_reuse = 1
	net.ipv4.tcp_tw_recycle = 1
	

### 客户端模拟并发
client端频繁建立连接，而端口释放较慢，导致建立新连接时无可用端口。

	#修改临时端口范围、端口释放时间、端口复用
	net.ipv4.tcp_fin_timeout=30
	net.ipv4.tcp_timestamps=1
	net.ipv4.tcp_tw_reuse=1
	net.ipv4.ip_local_port_range = 1024 65535


	#iptable防火墙对最大跟踪的TCP连接数有限制
	net.netfilter.nf_conntrack_max = 1048756
	net.nf_conntrack_max = 1048756



