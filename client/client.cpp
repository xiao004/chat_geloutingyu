#include <stdio.h>

#include "listen_pthread_param.h"
#include "client.h"


// uid_sockfd[i] 表示本用户与 uid 为 i 的用户当的通信 sockfd
int uid_sockfd[65536];

// uid_client_b_sockfd[client_b_sockfd] 表连接 client_b_sockfd 对应的用户 uid
int uid_client_b_sockfd[65536];

// 维护一个全局的 client_b_sockfd 的值,供线程函数使用
int client_b_sockfd_thread = 0;


client* client::instance = NULL;


void print_msg(message *msg, sockaddr_in *m_address) {

	printf("---------------------------------\r\n");
	printf("receive a pkg from %s:%d, the content of this pkg is:\n", inet_ntoa(m_address->sin_addr), ntohs(m_address->sin_port));
	printf("%d\n", (msg->head).type);
	printf("%d\n", (msg->head).length);
	printf("%s\n", msg->body);
	printf("---------------------------------\r\n");
}


// 用 static 修饰的函数，限定在本源码文件中，不能被本源码文件以外的代码文件调用。而普通的函数，默认是 extern 的，也就是说它可以被其它代码文件调用
// 其他文件中可以定义相同名字的函数，不会发生冲突
// 静态函数不能被其他文件所用
// 子进程会复制父进程中的静态数据(还有堆数据, 栈数据)

// 将 fd 设置为非阻塞的
static int setnonblocking(int fd) {
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;

	fcntl(fd, F_SETFL, new_option);

	return old_option;
}


// 在 epollfd 内核事件表中注册 fd 上的 EPOLLIN 事件且开启 ET 模式
static void addfd(int epollfd, int fd) {
	epoll_event event;

	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;

	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

	// epoll 的 et 模式下注册事件所属文件描述符必须为非阻塞的
	setnonblocking(fd);
}


// 从 epollfd 内核事件表中删除 fd 上的所有注册事件
static void removefd(int epollfd, int fd) {
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}


// 默认构造函数实现
client::client() {

	memset(uid_sockfd, -1, sizeof(uid_sockfd));

	epollfd = epoll_create(5);
	assert(epollfd != -1);

	// 初始化 m_uid 为 0，表当前用户没有登录
	m_uid = 0;

	// 初始化 m_uid 对应的密码为 ""
	m_password = "";

	// 初始化监听和到 route server, work server, client b 的 sockfd
	listen_sockfd = -1;
	route_sockfd = -1;
	work_sockfd = -1;
	client_b_sockfd = -1;

	// 初始化 listen_addr, route_addr, server_addr, client_b_addr
	listen_addr.sin_port = -1;
	route_addr.sin_port = -1;
	work_addr.sin_port = -1;
	client_b_addr.sin_port = -1;
}


// 析构函数实现
client::~client() {

}


// 创建 listen_sockfd, route_sockfd, work_sockfd 和 client_b_sockfd 并将它们绑定到 listen_addr
void client::create_sockfds() {
	if(listen_addr.sin_port == -1) {
		std::cout << "error, after do create_sockfds you should aready do set_listen_addr!!!" << std::endl;
		return;
	}

	// 初始化监听和到 route server, work server, client b 的 sockfd
	// 监听 socket
	listen_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listen_sockfd >= 0);

	// 向 route server 发送信息的 socket
	route_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(route_sockfd);

	// 向 work server 发送信息的 socket
	work_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(work_sockfd);

	// // 向 client_b_sockfd 发送信息的 socket
	// client_b_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	// assert(client_b_sockfd);

	// 四个 socket 都要设置端口复用
	int opt = 1;
	int ret = -1;
	ret = setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	assert(ret != -1);

	ret = setsockopt(route_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	assert(ret != -1);

	ret = setsockopt(work_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	assert(ret != -1);

	// ret = setsockopt(client_b_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	// assert(ret != -1);

	// 在 listen 之前对四个 socket 进行 bind
	ret = bind(listen_sockfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
	assert(ret != -1);

	ret = bind(route_sockfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
	assert(ret != -1);

	ret = bind(work_sockfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
	assert(ret != -1);

	// ret = bind(client_b_sockfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
	// assert(ret != -1);

	// ret = listen(listen_sockfd, 5);
	// assert(ret != -1);
}


// 设置 listen_addr
void client::set_listen_addr(const char *ip, const int port) {
	listen_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &listen_addr.sin_addr);
	listen_addr.sin_port = htons(port);
}


// 设置 route_addr
void client::set_route_addr(const char *ip, const int port) {
	route_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &route_addr.sin_addr);
	route_addr.sin_port = htons(port);
}


// 设置 work_addr
void client::set_work_addr(const char *ip, const int port) {
	work_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &work_addr.sin_addr);
	work_addr.sin_port = htons(port);	
}

// 设置 client_b_addr
void client::set_client_b_addr(const char *ip, const int port) {
	client_b_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &client_b_addr.sin_addr);
	client_b_addr.sin_port = htons(port);	
}


// 设置 work_sockfd
void client::set_work_sockfd(int work_server_sockfd) {
	work_sockfd = work_server_sockfd;
}


// 用 route_sockfd 向 route server 发起 connect 并返回 connect 后的 sockfd
int client::init_route_sockfd() {
	if(route_addr.sin_port == -1) {
		printf("you have not init route_addr, please use client::create_route_addr to do it!\n");
		return -1;
	}

	if(route_sockfd < 1) {
		printf("you have not create route_sockfd, please use client::create_sockfds to do it!\n");
		return -1;
	}

	if(connect(route_sockfd, (struct sockaddr*)&route_addr, sizeof(route_addr)) < 0) {
		printf("connection failed\n");
		close(route_sockfd);
		return -1;
	}

	return route_sockfd;
}


// 关闭连接 route_sockfd
void client::delete_route_sockfd() {
	close(route_sockfd);
	route_sockfd = -1;
}


// 从 route_sockfd 获取 work server addr，
// 并用 work_sockfd 向 work server 发起 connect 并返回 connect 后的 sockfd
int client::init_work_sockfd() {
	
	if(work_sockfd < 1) {
		printf("you have not create work_sockfd, please use client::create_sockfds to do it!\n");
		return -1;
	}

	memset(buf, '\0', BUFFER_SIZE);

	// 发送一个 type=0, buf=NULL 的报文
	send_msg(route_sockfd, OBTAIN_SERVER_ADDR, NULL);

	// 获取 route server 返回的数据
	recv(route_sockfd, buf, BUFFER_SIZE - 1, 0);

	struct message *msg = (message*)buf;

	// printf("%d\n", (msg->head).type);
	// printf("%d\n", (msg->head).length);
	// printf("%s\n", msg->body);
	// print_msg(msg, &route_addr);

	if((msg->head).type == ERROR) {
		printf("error, %s\n", msg->body);
		return -1;
	}

	char *idx = strchr(msg->body, ':');
	char server_ip[17];
	int server_port = atoi(idx + 1);
	strcpy(server_ip, msg->body);
	server_ip[idx - msg->body] = '\0';
	
	// printf("%s:%d\n", server_ip, server_port);

	set_work_addr(server_ip, server_port);

	// struct sockaddr_in server_address;
	// bzero(&server_address, sizeof(server_address));
	// server_address.sin_family = AF_INET;
	// inet_pton(AF_INET, server_ip, &server_address.sin_addr);
	// server_address.sin_port = htons(server_port);

	// server_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	// assert(server_sockfd >= 0);

	// close(route_sockfd);

	if(work_addr.sin_port == -1) {
		printf("you have not init route_addr, please use client::create_work_addr to do it!\n");
		return -1;
	}

	if(connect(work_sockfd, (struct sockaddr*)&work_addr, sizeof(work_addr)) < 0) {
		printf("connection failed\n");
		close(work_sockfd);
		work_sockfd = -1;
		return -1;
	}

	return work_sockfd;
}

// 从 work_sockfd 获取 clien_b addr，
// 并用 client_b_sockfd 向 client b 发起 connect 并返回 connect 后的 sockfd
int client::init_client_b_sockfd(const int uid) {
	if(work_sockfd < 1) {
		printf("you have not create work_sockfd, please use client::create_sockfds to do it!\n");
		return -1;
	}

	memset(buf, '\0', BUFFER_SIZE);

	snprintf(buf, BUFFER_SIZE, "%d:%s:%d", m_uid, m_password.c_str(), uid);

	// 发送一个 type=0, buf=m_uid:m_password:uid 的报文
	send_msg(work_sockfd, OBTAIN_CLIENT_ADDR, buf);

	// 获取 work server 返回的数据
	recv(work_sockfd, buf, BUFFER_SIZE - 1, 0);

	struct message *msg = (message*)buf;

	// printf("%d\n", (msg->head).type);
	// printf("%d\n", (msg->head).length);
	// printf("%s\n", msg->body);
	// print_msg(msg, &route_addr);

	if((msg->head).type == ERROR) {
		printf("error, %s\n", msg->body);
		return -1;
	}

	char *idx = strchr(msg->body, ':');
	char server_ip[17];
	int server_port = atoi(idx + 1);
	strcpy(server_ip, msg->body);
	server_ip[idx - msg->body] = '\0';
	
	// printf("%s:%d\n", server_ip, server_port);

	set_client_b_addr(server_ip, server_port);

	// struct sockaddr_in server_address;
	// bzero(&server_address, sizeof(server_address));
	// server_address.sin_family = AF_INET;
	// inet_pton(AF_INET, server_ip, &server_address.sin_addr);
	// server_address.sin_port = htons(server_port);

	// server_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	// assert(server_sockfd >= 0);

	// close(route_sockfd);

	// std::cout << uid << " " << uid_sockfd[uid] << std::endl;

	// std::cout << client_b_sockfd << std::endl;

	if(client_b_addr.sin_port == -1) {
		printf("you have not init route_addr, please use client::create_work_addr to do it!\n");
		return -1;
	}

	// 如果 m_uid 和 uid 用户之间的信道已经开启,则直接使用对应的 sockfd 即可
	if(uid_sockfd[uid] != -1) {
		client_b_sockfd = uid_sockfd[uid];

		// 将 client_b_sockfd 注册到 epoll 内核事件表
		addfd(epollfd, client_b_sockfd);

		// std::cout << client_b_sockfd << "---" << std::endl;

		return client_b_sockfd;
	}

	// close 后要重新创建向 client_b_sockfd 发送信息的 socket
	client_b_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(client_b_sockfd != -1);

	if(connect(client_b_sockfd, (struct sockaddr*)&client_b_addr, sizeof(client_b_addr)) < 0) {
		printf("client_b_sockfd connection failed\n");
		close(client_b_sockfd);
		client_b_sockfd = -1;
		return -1;
	}

	// 记录 uid 对应的 sockfd
	uid_sockfd[uid] = client_b_sockfd;

	// 将 client_b_sockfd 注册到 epoll 内核事件表
	addfd(epollfd, client_b_sockfd);

	return client_b_sockfd;
}


// 将 client_b_sockfd 从 epoll 移除并关闭该连接
void client::delete_client_b_sockfd() {
	removefd(epollfd, client_b_sockfd);

	uid_sockfd[client_b_sockfd] = -1;

	close(client_b_sockfd);
}


// 对 listen_sockfd 进行 listen 系统调用
// 并将 listen_sockfd 注册到 epoll 内核事件表
int client::init_listen_sockfd() {
	int ret = listen(listen_sockfd, 5);
	assert(ret != -1);

	addfd(epollfd, listen_sockfd);
}


// 将 listen_sockfd 从 epoll 移除并关闭连接
void client::delete_listen_sockfd() {
	removefd(epollfd, listen_sockfd);

	close(listen_sockfd);
}


// 清除 epoll 内核表中注册的 sockfd
void client::clean_epollfd() {
	for(int i = 0; i < 65536; ++i) {
		if((uid_sockfd[i] != -1) && (i != listen_sockfd)) {
			int sockfd = uid_sockfd[i];

			removefd(epollfd, sockfd);

			close(sockfd);

			uid_sockfd[i] = -1;
		}
	}
}


// 监听线程内容函数
void* client::listen_pthread(void *data) {
	int ret = 0;

	const int BUFFER_SIZE = 1024;

	char buf[BUFFER_SIZE];

	auto param = (listen_pthread_param*)data;

	epoll_event events[MAX_EVENT_NUMBER];

	struct sockaddr_in client_address;

	int uid = 0;

	while(true) {
		int number = epoll_wait(param->epollfd, events, MAX_EVENT_NUMBER, -1);

		if((number < 0) && (errno != EINTR)) {
			printf("epoll failure\n");
			break;
		}

		// 遍历触发的事件
		for(int i = 0; i < number; ++i) {

			int sockfd = events[i].data.fd;

			// std::cout << "==========" << sockfd << std::endl;

			// 有新的 client 连接到来
			if(sockfd == param->listen_sockfd) {

				socklen_t client_addrlength = sizeof(client_address);

				int connfd = accept(param->listen_sockfd, (struct sockaddr*)&client_address, &client_addrlength);

				addfd(param->epollfd, connfd);

			} else if(events[i].events & EPOLLIN) { 

				ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
				// printf("%d\n", ret);


				// printf("event trigger once--------------------------------------\n");
				// // et 事件不会被重复触发, 需要循环读取数据, 确保把当前 socket 读缓存中的所有数据读出

				// while(true) {
				// 	memset(buf, '\0', BUFFER_SIZE);

				// 	int ret = recv(sockfd, buf, BUFFER_SIZE-1, 0);
					
				// 	if(ret < 0) {
				// 		if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				// 			printf("raed later\n");
				// 			break;
				// 		}

				// 		removefd(param->epollfd, sockfd);
				// 		close(sockfd);

				// 		break;

				// 	} else if(ret == 0) {
				// 		removefd(param->epollfd, sockfd);
				// 		close(sockfd);

				// 	} else {
				// 			buf[ret] = '\0';

				// 			struct message *msg = (message*)buf;

				// 			print_msg(msg, &client_address);

				// 			switch((msg->head).type) {
				// 				case USER_CONVERSATION: {
				// 					// std::cout << msg->body << std::endl;
				// 					std::string body = msg->body;

				// 					auto idex = body.find(':');

				// 					uid = atoi((body.substr(0, idex)).c_str());

				// 					std::cout << uid << "->me: " << body.substr(idex + 1) << std::endl;

				// 					uid_sockfd[uid] = sockfd;

				// 					std::cout << "-----" << sockfd << std::endl;

				// 					// send_msg(sockfd, USER_CONVERSATION, "accept success~");

				// 					break;
				// 				}

				// 				default: {
				// 					break;
				// 				}
				// 			}
				// 	}
				// }

				// 如果读操作发生错误，则关闭客户连接．但是如果是暂时无数据可读，则退出循环
				if(ret < 0) {
					// EAGAIN 表连接没有完全建立但正在建立中．通常发生在客户端发起异步 connect 的场景中
					// 对于这种情况直接 break，等待下一次事件触发即可
					// 对于其他错误情况则断开连接
					if(errno != EAGAIN) {
						removefd(param->epollfd, sockfd);

						// std::cout << uid << "ppppp" << std::endl;

						uid_sockfd[uid] = -1;

						// 当前客户端只发送了消息给对方,但是没有接收到对方的消息则会出现 uid = 0
						if(uid == 0 && client_b_sockfd_thread >= 0) {
							// std::cout << uid_client_b_sockfd[client_b_sockfd_thread] << std::endl;

							uid_sockfd[uid_client_b_sockfd[client_b_sockfd_thread]] = -1;
						}

						close(sockfd);
					}

					// std::cout << "ret < 0" << std::endl;

					break;

				} else if(ret == 0) {
					removefd(param->epollfd, sockfd);

					// std::cout << uid << "qqqq" << std::endl;

					uid_sockfd[uid] = -1;

					// 当前客户端只发送了消息给对方,但是没有接收到对方的消息则会出现 uid = 0
					if(uid == 0 && client_b_sockfd_thread >= 0) {
						// std::cout << uid_client_b_sockfd[client_b_sockfd_thread] << std::endl;

						uid_sockfd[uid_client_b_sockfd[client_b_sockfd_thread]] = -1;
					}

					close(sockfd);

					// std::cout << "ret == 0" << std::endl;

					break;

				} else {
					buf[ret] = '\0';

					struct message *msg = (message*)buf;

					// print_msg(msg, &client_address);

					switch((msg->head).type) {
						case USER_CONVERSATION: {
							// std::cout << msg->body << std::endl;
							std::string body = msg->body;

							auto idex = body.find(':');

							uid = atoi((body.substr(0, idex)).c_str());

							std::cout << uid << "->me: " << body.substr(idex + 1) << std::endl;

							uid_sockfd[uid] = sockfd;

							// std::cout << "-----" << sockfd << std::endl;

							// send_msg(sockfd, USER_CONVERSATION, "accept success~");

							break;
						}

						default: {
							break;
						}
					}
				}

			} else {
				printf("something else happened\n");
			}
		}
	}
}


// 监听其它 client 发送过来的消息
void client::listen_msg() {
	int ret;

	listen_pthread_param *param = new listen_pthread_param;
	param->epollfd = epollfd;
	param->listen_sockfd = listen_sockfd;
	param->client_b_sockfd = client_b_sockfd;

	// 另开一个线程用于监听其它 client 发送过来的消息
	pthread_t listen_pid;

	ret = pthread_create(&listen_pid, NULL, listen_pthread, (void*)param);
	// pthread_join(report_connect_pid, NULL);

	// 释放 param 指针?
}


// 发送消息给 client_b
void client::send_msg_to_clien_b(const int uid) {
	std::string msg_format = "%d:%s";
	std::string msg;

	while(true) {
		std::cout << ">>>";
		std::cin >> msg;

		if(msg == "quit") {
			break;
		}

		// 每次发送消息前都要检查对方是否在线
		if(check_user_online(uid) != true) {
			std::cout << "usre: " << uid << " is not online, can't send message to him~" << std::endl;
			continue;
		}

		// std::cout << buf << "++++" << std::endl;

		// std::cout << client_b_sockfd << "===" << std::endl;

		// std::cout << uid << "---" << uid_sockfd[uid] << std::endl;

		// 每次发送消息前都要检查并更新 client_b_sockfd
		if(uid_sockfd[uid] == -1) {
			init_client_b_sockfd(uid);
		}

		// memset(buf, '\0', sizeof(buf));

		// 因为 check_user_online 和 init_client_b_sockfd 中都使用了 buf,因此 snprintf 步骤要放到其后
		snprintf(buf, sizeof(buf), msg_format.c_str(), m_uid, msg.c_str());

		// // 检查发送的消息是否正确
		// std::cout << buf << std::endl;

		// 发送消息给 client_b
		send_msg(client_b_sockfd, USER_CONVERSATION, buf);

		// 记录 client_b_sockfd 对应的 uid
		if(client_b_sockfd >= 0) {
			// std::cout << uid << ";;;" << std::endl;

			uid_client_b_sockfd[client_b_sockfd] = uid;

			client_b_sockfd_thread = client_b_sockfd;
		}

		std::cout << "me->" << uid << ": " << msg << std::endl;
	}
}


// 检查用户是否在线
// 在线返回 true，否则返回 false
bool client::check_user_online(const int who) {
	memset(buf, '\0', BUFFER_SIZE);
	snprintf(buf, BUFFER_SIZE, "%d:%s:%d", m_uid, m_password.c_str(), who);

	// 向 work_sockfd 发送一个 type 为 FIND_USER 内容为 "uid:password:who" 的报文，用于退出登录
	send_msg(work_sockfd, FIND_USER, buf);

	memset(buf, '\0', BUFFER_SIZE);

	// 获取 work server 返回的数据
	recv(work_sockfd, buf, BUFFER_SIZE - 1, 0);

	struct message *msg = (message*)buf;

	// print_msg(msg, &route_addr);

	return atoi(msg->body);
}


// 展示用户列表
// 参数 page 表第 page 页的用户列表，cap 表每页的容量
// 返回查找到的页表
char* client::user_list(const int page, const int cap) {
	memset(buf, '\0', BUFFER_SIZE);
	snprintf(buf, BUFFER_SIZE, "%d:%d", page, cap);

	// 向 work_sockfd 发送一个 type 为 USER_LIST 内容为 "page:cap" 的报文，用于退出登录
	send_msg(work_sockfd, USER_LIST, buf);

	memset(buf, '\0', BUFFER_SIZE);

	// 获取 work server 返回的数据
	recv(work_sockfd, buf, BUFFER_SIZE - 1, 0);

	struct message *msg = (message*)buf;

	// print_msg(msg, &route_addr);
	return msg->body;
}


// 退出登录
// 退出登录成功返回 true，失败返回 false
bool client::logout() {
	memset(buf, '\0', BUFFER_SIZE);

	snprintf(buf, BUFFER_SIZE, "%d:%s", m_uid, m_password.c_str());

	// 向 work_sockfd 发送一个 type 为 LOGOUT 内容为 "" 的报文，用于退出登录
	send_msg(work_sockfd, LOGOUT, buf);

	memset(buf, '\0', BUFFER_SIZE);

	// 获取 work server 返回的数据
	recv(work_sockfd, buf, BUFFER_SIZE - 1, 0);

	struct message *msg = (message*)buf;

	// print_msg(msg, &route_addr);

	if((msg->head).type == ERROR) {
		printf("error, %s\n", msg->body);
		return false;
	}

	// 返回 0/1，会自动转换为 bool 类型
	return atoi(msg->body);
}


// 登录账号
// 登录成功返回 true，登录失败返回 false
bool client::login(const int uid, const std::string &password) {
	memset(buf, '\0', BUFFER_SIZE);

	snprintf(buf, BUFFER_SIZE, "%d:%s", uid, password.c_str());

	// printf("%s\n", buf);

	// 向 work_sockfd 发送一个 type 为 LOGIN 内容为 buf 的报文，用于登录账号
	send_msg(work_sockfd, LOGIN, buf);

	memset(buf, '\0', BUFFER_SIZE);

	// 获取 work server 返回的数据
	recv(work_sockfd, buf, BUFFER_SIZE - 1, 0);

	struct message *msg = (message*)buf;

	// print_msg(msg, &route_addr);

	if((msg->head).type == ERROR) {
		printf("error, %s\n", msg->body);
		return false;
	}

	bool ret = atoi(msg->body);

	// 登录成功则将 m_uid 设置为 uid
	if(ret) {
		m_uid = uid;
		m_password = password;
	}

	return ret;
}


// 创建群组
// 创建成功则返回注册的 gid (大于 0)，失败则返回 0
int client::create_group(const std::string& gname) {
	memset(buf, '\0', BUFFER_SIZE);

	snprintf(buf, BUFFER_SIZE, "%d:%s:%s", m_uid, m_password.c_str(), gname.c_str());

	// 向 work_sockfd 发送一个 type 为 CREATE_GROUP 内容为 buf 的报文，用于注册账号
	send_msg(work_sockfd, CREATE_GROUP, buf);

	memset(buf, '\0', BUFFER_SIZE);

	// 获取 work server 返回的数据
	recv(work_sockfd, buf, BUFFER_SIZE - 1, 0);

	struct message *msg = (message*)buf;

	// print_msg(msg, &route_addr);

	if((msg->head).type == ERROR) {
		printf("error, %s\n", msg->body);
		return 0;
	}

	return atoi(msg->body);
}


// 注册账号
// 注册成功则返回注册的 uid (大于 0)，失败则返回 0
int client::registered(const std::string &uname, const std::string &password) {
	memset(buf, '\0', BUFFER_SIZE);

	snprintf(buf, BUFFER_SIZE, "%s:%s", uname.c_str(), password.c_str());

	// 向 work_sockfd 发送一个 type 为 REGISRERED 内容为 buf 的报文，用于注册账号
	send_msg(work_sockfd, REGISRERED, buf);

	memset(buf, '\0', BUFFER_SIZE);

	// 获取 work server 返回的数据
	recv(work_sockfd, buf, BUFFER_SIZE - 1, 0);

	struct message *msg = (message*)buf;

	// print_msg(msg, &route_addr);

	if((msg->head).type == ERROR) {
		printf("error, %s\n", msg->body);
		return 0;
	}

	return atoi(msg->body);

}


// 删除账号
// 删除成功返回 true, 失败则返回 false
bool client::delete_user(const int uid, const std::string &password) {
	memset(buf, '\0', BUFFER_SIZE);

	snprintf(buf, BUFFER_SIZE, "%d:%s", uid, password.c_str());

	// printf("%s\n", buf);

	// 向 work_sockfd 发送一个 type 为 DELETE_USER 内容为 buf 的报文，用于登录账号
	send_msg(work_sockfd, DELETE_USER, buf);

	memset(buf, '\0', BUFFER_SIZE);

	// 获取 work server 返回的数据
	recv(work_sockfd, buf, BUFFER_SIZE - 1, 0);

	struct message *msg = (message*)buf;

	// print_msg(msg, &route_addr);

	if((msg->head).type == ERROR) {
		printf("error, %s\n", msg->body);
		return false;
	}

	return atoi(msg->body);
}


// // 往 sockfd 发送类型为 type 内容为 buf 的报文
// int client::send_msg(const int sockfd, const int type, const char *buf) {

// 	struct message msg;
// 	msg.head.type = type;
// 	msg.head.length = MSG_HEAND;

// 	if(buf != NULL) {
// 		memcpy(&msg.body, buf, strlen(buf) + 1);

// 		msg.head.length += strlen(buf) + 1;
// 	}

// 	send(sockfd, (char*)&msg, msg.head.length, 0);

// 	// printf("%s\n", msg.body);
// 	// printf("%d\n", msg.head.length);

// 	// switch(type) {
// 	// 	case OBTAIN_SERVER_ADDR: {

// 	// 	}
// 	// }
// }
