// 客户端类

#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
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

#include "../internal/message.h"

// epoll 最多监听的 sockfd 数目
#define MAX_EVENT_NUMBER 1024


class client {

private:
	client();

	// 监听线程内容函数
	static void* listen_pthread(void *data);

	// 设置 work_addr
	void set_work_addr(const char *ip, const int port);

public:
	// 单列模式创建对象
	static client* create() {
		if(instance == NULL) {
			instance = new client();
		}

		return instance;
	}

	// 创建群组
	// 创建成功则返回注册的 gid (大于 0)，失败则返回 0
	int create_group(const std::string& gname);

	// 监听其它 client 发送过来的消息
	void listen_msg();

	// 发送消息给 client_b
	void send_msg_to_clien_b(const int uid);

	// 检查用户是否在线
	// 在线返回 true，否则返回 false
	bool check_user_online(const int uid);

	// 展示用户列表
	// 参数 page 表第 page 页的用户列表，cap 表每页的容量
	// 返回查找到的页表
	char* user_list(const int page, const int cap = 20);

	// 退出登录
	// 退出登录成功返回 true，失败返回 false
	bool logout();

	// 登录账号
	// 登录成功返回 true，登录失败返回 false
	bool login(const int uid, const std::string &password);

	// 注册账号
	// 注册成功则返回注册的 uid (大于 0)，失败则返回 0
	int registered(const std::string &uname, const std::string &password);

	// 删除账号
	// 删除成功返回 true, 失败则返回 false
	bool delete_user(const int uid, const std::string &password);

	// 设置 route_addr
	void set_route_addr(const char *ip, const int port);

	// 设置 work_sockfd
	void set_work_sockfd(int work_server_sockfd);

	// 设置 listen_addr
	void set_listen_addr(const char *ip, const int port);

	// 设置 client_b_addr
 	void set_client_b_addr(const char *ip, const int port);

	// 用 route_sockfd 向 route server 发起 connect 并返回 connect 后的 sockfd
	int init_route_sockfd();

	// 关闭连接 route_sockfd
	void delete_route_sockfd();

	// 从 route_sockfd 获取 work server addr，
	// 并用 work_sockfd 向 work server 发起 connect 并返回 connect 后的 sockfd
	int init_work_sockfd();

	// 从 work_sockfd 获取 clien_b addr，
	// 并用 client_b_sockfd 向 client b 发起 connect 并返回 connect 后的 sockfd
	int init_client_b_sockfd(const int uid);

	// 将 client_b_sockfd 从 epoll 移除并关闭该连接
	void delete_client_b_sockfd();

	// 对 listen_sockfd 进行 listen 系统调用并将 listen_sockfd 注册到 epoll 事件内核表
	int init_listen_sockfd();

	// 将 listen_sockfd 从 epoll 移除并关闭 listen_sockfd 连接
	void delete_listen_sockfd();

	// 创建 listen_sockfd, route_sockfd, work_sockfd 和 client_b_sockfd 并将它们绑定到 listen_addr
	void create_sockfds();

	// 清除 epoll 内核表中注册的 sockfd
	void clean_epollfd();

	// 往 sockfd 发送类型为 type 内容为 buf 的报文
	// int send_msg(const int sockfd, const int type, const char *buf);
	
	~client();

private:
	// 当前 client 的 uid
	int m_uid;

	// m_uid 对应的密码
	std::string m_password;

	// 缓冲区大小
	static const int BUFFER_SIZE = 1024;

	// 本地监听, route server, work server, client b 的地址
	struct sockaddr_in listen_addr, route_addr, work_addr, client_b_addr;

	// epoll 内核表文件描述符
	int epollfd;

	// 监听以及到 route server, work server, client b 的 sockfd
	int listen_sockfd, route_sockfd, work_sockfd, client_b_sockfd;

	// 数据缓冲区
	char buf[BUFFER_SIZE];

	// 单列对象
	static client *instance;
	
};



#endif