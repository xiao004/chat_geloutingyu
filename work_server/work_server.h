// work server 的业务逻辑类

#ifndef WORK_SERVER_H
#define WORK_SERVER_H

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
#include <iostream>

#include "work_processpool.h"
#include "../internal/message.h"


// 该类是在子进程中实列化并初始化的，其主要抽象了业务逻辑代码
// 子进程中每加入一个新连接，则初始化一个 work_server 对象
class work_server {

public:
	work_server(){}
	~work_server(){}

	// 初始化 work_serser 对象
	void init(int epollfd, int sockfd, const sockaddr_in& client_addr, int *share_mem, int sem_id);

	// 逻辑业务函数
	void process();

private:
	// op 为 -1 时执行 p 操作, op 为 1 时执行 v 操作
	void pv(int sem_id, int op);

	// 连接计数加一
	void share_mem_increase();

	// 连接计数减一
	void share_mem_reduce();

	// 创建群组
	// 创建成功则返回注册的 gid (大于 0)，失败则返回 0
	int create_group(const std::string &uid_password_gname);


	// 获取 client 的登录地址并返回该结果
	const char* obtain_client_addr(const std::string &uid_password_client);

	// 检查 uid 为 who 的用户是否在线
	// 在线返回 true，否则返回 false
	bool check_user_online(const std::string &uid_password_who);

	// 展示用户列表
	// 返回查找到的用户列表
	const char* user_list(const std::string &page_cap);

	// 退出登录
	// 退出登录成功返回 true, 失败则返回 false
	bool logout(const std::string &uid_password);

	// 登录账号
	// 登录成功返回 true，失败则返回 false
	bool login(const std::string &uid_password, const std::string &addr);

	// 注册账号
	// 注册成功则返回注册的 uid (大于 0)，失败则返回 0
	int registered(std::string uname_password);

	// 删除账号
	// 删除成功返回 true, 失败则返回 false
	bool delete_user(const std::string uid_password);

	// 判断用执行操作的户是否登录
	// 登录了返回 true, 否则返回 false
	bool is_login(const int uid, const std::string &password);


private:
	// 缓冲区大小
	static const int BUFFER_SIZE = 1024;

	// 初始化该类的子进程中的 epoll 内核事件表文件描述符
	static int m_epollfd;

	// 共享内存地址 & 记录当前机器的用户连接数
	int *share_mem;

	// 该子进程监听的 socket 连接文件描述符
	int m_sockfd;

	// 对应客户端地址
	sockaddr_in m_address;

	// 对应客户端 uid
	int m_uid;

	// 对应客户端 uid 的密码
	std::string m_password;

	// 数据缓冲区
	char m_buf[BUFFER_SIZE];

	// 标记缓冲区已读如客户数据的最后一个字节的下一个位置
	int m_read_idx;

	// 信号量集标识符
	int sem_id;
	
};


#endif