#include <string.h>

#include "route_server.h"
#include "route_processpool.h"
#include "../internal/message.h"
#include "../internal/work_server_addr.h"


// 类外初始化静态成员变量
int route_server::m_epollfd = -1;


// 打印客户端信息
void print_sockaddr_in(struct sockaddr_in *address) {
	printf("==========================================================\r\n");
    printf("a new socket client add, the infomation of this client is:\n");
    printf("sin_addr : %s\n", inet_ntoa(address->sin_addr));
    printf("sin_port : %d\n", ntohs(address->sin_port));
    printf("sin_family : %d\n", address->sin_family);
    printf("==========================================================\r\n");
}


void print_share_mem(struct work_server_addr *server_addr) {
	printf("+++++++++++++++++++++++\r\n");
	printf("%s\n", (server_addr->addr).ip);
	printf("%d\n", (server_addr->addr).port);
	printf("%d\n", server_addr->connections);
	printf("+++++++++++++++++++++++\r\n");
}


void print_msg(message *msg, sockaddr_in *m_address) {

	printf("---------------------------------\r\n");
	printf("receive a pkg from %s:%d, the content of this pkg is:\n", inet_ntoa(m_address->sin_addr), ntohs(m_address->sin_port));
	printf("%d\n", (msg->head).type);
	printf("%d\n", (msg->head).length);
	printf("%s\n", msg->body);
	printf("---------------------------------\r\n");
}


// route_server 类初始化成员函数实现
void route_server::init(int epollfd, int sockfd, const sockaddr_in& client_addr, work_server_addr *share_mem_) {
	m_epollfd = epollfd;
	m_sockfd = sockfd;
	m_address = client_addr;
	share_mem = share_mem_;

	memset(m_buf, '\0', BUFFER_SIZE);

	m_read_idx = 0;
}


// route_server 业务逻辑成员函数实现
void route_server::process() {
	int idx = 0;
	int ret = -1;

	while(true) {
		idx = m_read_idx;

		// memset(m_buf, '\0', BUFFER_SIZE);
		ret = recv(m_sockfd, m_buf, BUFFER_SIZE - 1, 0);
		// printf("%d\n", ret);

		// work_server_addr *server_addr = share_mem + m_sockfd;

		// 如果读操作发生错误，则关闭客户连接．但是如果是暂时无数据可读，则退出循环
		if(ret < 0) {
			// EAGAIN 表连接没有完全建立但正在建立中．通常发生在客户端发起异步 connect 的场景中
			// 对于这种情况直接 break，等待下一次事件触发即可
			// 对于其他错误情况则断开连接
			if(errno != EAGAIN) {
				// 如果该连接是 work server, 则将共享内存中对应对象的 connections 置-1
				// server_addr->connections = -1;
				rmove_server_addr(m_sockfd);

				removefd(m_epollfd, m_sockfd);

			}
			break;

		} else if(ret == 0) {
			// 如果该连接是 work server, 则将共享内存中对应对象的 connections 置-1
			// server_addr->connections = -1;
			rmove_server_addr(m_sockfd);

			removefd(m_epollfd, m_sockfd);

			break;

		} else {
			m_buf[ret] = '\0';

			struct message *msg = (message*)m_buf;

			print_msg(msg, &m_address);

			switch((msg->head).type) {
				// 接收到请求获取 work_server address 的报文
				case OBTAIN_SERVER_ADDR: {
					// printf("the work server list at now is:\n");
					// print_work_server_list();

					// 获取当前连接数最少的 work server addr
					address min_connections_server = get_min_connections_server();

					if(min_connections_server.port == -1) {
						printf("there are none server now!!!\n");

						// 若当前没有在线的 work server 则回复 client 一个错误报文
						ret = send_msg(m_sockfd, ERROR, "now there none work server~\0");

						break;
					}

					printf("the min connections server is: %s:%d\n", min_connections_server.ip, min_connections_server.port);

					char addr[25];
					memcpy(addr, min_connections_server.ip, strlen(min_connections_server.ip) + 1);
					sprintf(addr + strlen(addr), ":%d", min_connections_server.port);

					printf("%s\n", addr);

					// 回复 client 一个当前连接数最少的 work_server address
					ret = send_msg(m_sockfd, (msg->head).type, addr);

					break;
				}

				case REPORT_CONNECT: {

					// 接受到某台 work server 的汇报连接数报文, 则修改对应的 work_server_addr 对象的 connections
					set_server_connections(m_sockfd, atoi(msg->body));

					// printf("%d\n", m_sockfd);

					break;
				}

				default: {
					break;
				}
			}

		}

	}
}


// 打印当前的 work server 列表
void route_server::print_work_server_list(void) {
	for(int i = 0; i < MAX_WORK_SERVER; ++i) {
		work_server_addr *cnt = share_mem + i;

		if(cnt->connections != -1) {
			print_share_mem(cnt);
		}
	}
}


// 获取当前连接数最少的 work server
address route_server::get_min_connections_server(void) {
	address ret = {"\0", -1};
	int min = -1;

	for(int i = 0; i < MAX_WORK_SERVER; ++i) {
		work_server_addr *cnt = share_mem + i;

		if(min == -1 && cnt->connections != -1) {
			min = cnt->connections;
			ret = cnt->addr;

		} else if(cnt->connections != -1 && cnt->connections < min) {
			min = cnt->connections;
			ret = cnt->addr;
		}
	}

	return ret;
}


// 判断 server_sockfd 对应的是否是某台 work server
// 某台 work server 离开, 修改共享内存中对应的 work_server_addr 对象的 connections 值为 -1
void route_server::rmove_server_addr(int server_sockfd) {

	work_server_addr *server_addr = share_mem + m_sockfd;

	// 如果 server_sockfd 对应的是某台 work server, 则将共享内存对应 work_server_addr 对象的 connections 值置 -1 表该 work server 离开
	if(server_addr->connections != -1) {
		
		server_addr->connections = -1;

		printf("work server %s:%d leave\n", (server_addr->addr).ip, (server_addr->addr).port);

		printf("the work server list at now is:\n");
		print_work_server_list();
	}
	
}


// 接受到某台 work server 的汇报连接数报文, 则修改对应的 work_server_addr 对象的 connections
// 如果其 connections 值为 -1, 即该 work server 是新加入的
void route_server::set_server_connections(int server_sockfd, const int new_connections) {

	work_server_addr *server_addr = share_mem + m_sockfd;

	// 是否有新的 work server 加入
	bool flag = false;

	// 若 connections 的值为 -1 即该 work server 是新加入的
	if(server_addr->connections == -1)	 {
		// m_address 即该客户连接的地址
		// 注意将网络字节序转为主机字节序
		const char *ip = inet_ntoa(m_address.sin_addr);
		memcpy((server_addr->addr).ip, ip, strlen(ip) + 1);
		(server_addr->addr).port = ntohs(m_address.sin_port);

		flag = true;
	}

	server_addr->connections = new_connections;

	if(flag) {
		printf("a new work server add, the addr of this server is:\n");
		print_share_mem(server_addr);

		printf("the work server list at now is:\n");
		print_work_server_list();
	}
}