//route_server 业务逻辑类

#ifndef ROUTE_SERVER_H
#define ROUTE_SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "../internal/work_server_addr.h"


// 该类是在子进程中实列化并初始化的，其主要抽象了业务逻辑代码
// 子进程中每加入一个新连接，则初始化一个 route_server 对象
class route_server {

public:
	route_server(){}
	~route_server(){}

	// 初始化 route_serser 对象
	void init(int epollfd, int sockfd, const sockaddr_in& client_addr, work_server_addr *share_mem);

	// 逻辑业务函数
	void process();

private:
	// 某台 work server 离开, 修改共享内存中对应的 work_server_addr 对象的 connections 值为 -1
	void rmove_server_addr(int server_sockfd);

	// 接受到某台 work server 的汇报连接数报文, 则修改对应的 work_server_addr 对象的 connections
	// 如果其 connections 值为 -1, 即该 work server 是新加入的
	void set_server_connections(int server_sockfd, const int new_connections);

	// 获取当前连接数最少的 work server
	address get_min_connections_server(void);

	// 打印当前的 work server 列表
	void print_work_server_list(void);


private:
	// 缓冲区大小
	static const int BUFFER_SIZE = 1024;

	// 最多负载的 work server 数量
	static const int MAX_WORK_SERVER = 65536;

	// 初始化该类的子进程中的 epoll 内核事件表文件描述符
	static int m_epollfd;

	// work_server 池 & 共享内存文件映射地址
	work_server_addr *share_mem;

	// 该子进程监听的 socket 连接文件描述符
	int m_sockfd;

	// 对应客户端地址
	sockaddr_in m_address;

	// 数据缓冲区
	char m_buf[BUFFER_SIZE];

	// 标记缓冲区已读如客户数据的最后一个字节的下一个位置
	int m_read_idx;
	
};


// // 类外定义和初始化静态成员变量
// int route_server::m_epollfd = -1; // 不能静态成员不能在头文件中定义，否则多个文件引用该头文件时该静态变量会被定义多次


#endif