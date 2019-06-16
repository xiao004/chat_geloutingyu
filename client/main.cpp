#include <stdio.h>
#include <string.h>
#include <iostream>
#include <regex>
#include <iomanip>

#include "../internal/message.h"
#include "client.h"
#include "../internal/work_server_addr.h"

using namespace std;

#define BUFFER_SIZE 1024


// 打印 sockaddr_in 中的信息
void print_sockaddr_in(struct sockaddr_in *address) {
	printf("==========================================================\r\n");
    // printf("a new socket client add, the infomation of this client is:\n");
    printf("sin_addr : %s\n", inet_ntoa(address->sin_addr));
    printf("sin_port : %d\n", ntohs(address->sin_port));
    printf("sin_family : %d\n", address->sin_family);
    printf("==========================================================\r\n");
}


// 检查字符串 str 是否小于 16bytes 且只由数字和字母组成
bool check_uname_or_password(const std::string &option, const std::string &str) {
	if(str.size() > 16) {
		std::cout << "error, " << option << " more than 16 bytes" << std::endl;
		return false;
	}

	regex reg("^[0-9a-zA-Z]+$");
	bool flag = regex_match(str, reg);

	if(!flag) {
		std::cout << "error, " << option << " can only be consisting of letters and numbers" << std::endl;
	}

	return flag;
}


// 检查字符串 uname 是否小于 16bytes 且只由数字和字母组成
bool check_uname(const std::string &uname) {
	return check_uname_or_password("user name", uname);
}


// 检查字符串 gname 是否小于 16bytes 且只由数字和字母组成
bool check_gname(const std::string &gname) {
	return check_uname_or_password("group name", gname);
}


// 检查字符串 password 是否小于 16bytes 且只由数字和字母组成
bool check_password(const std::string &password) {
	return check_uname_or_password("password", password);
}


// 和 ip:port 建立 socket 连接
// 成功则返回 socket 文件描述符，失败返回 -1
int get_connection(const char *ip, const int port) {

	struct sockaddr_in server_address;
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &server_address.sin_addr);
	server_address.sin_port = htons(port);

	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(socket >= 0);

	if(connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
		printf("connection failed\n");
		close(sockfd);
		return -1;
	}

	return sockfd;
}


int main(int argc, char const *argv[]) {

	if(argc <= 4) {
		printf("usage: %s, route_server ip, route_server port, client ip, client port\n", basename(argv[0]));
		return 1;
	}

	int ret = -1;

	// 创建 client 对象
	client *me = client::create();

	// 设置 listen_addr
	me->set_listen_addr(argv[3], atoi(argv[4]));

	// 设置 route_addr
	me->set_route_addr(argv[1], atoi(argv[2]));

	// 创建 listen_sockfd, route_sockfd, work_sockfd 和 client_b_sockfd 并将它们绑定到 listen_addr
	// 对 listen_sockfd 进行 listen 系统调用
	me->create_sockfds();

	// 用 route_sockfd 向 route server 发起 connect 并返回 connect 后的 sockfd
	ret = me->init_route_sockfd();
	assert(ret != -1);

	// 从 route_sockfd 获取 work server addr，
	// 并用 route_sockfd 向 route server 发起 connect 并返回 connect 后的 sockfd
	ret = me->init_work_sockfd();
	assert(ret != -1);

	// 获取 work server addr 后不再需要使用 route_sockfd，可以将其 close
	// 关闭 route_sockfd 连接
	me->delete_route_sockfd();

	// // 对 listen_sockfd 进行 listen 系统调用
	// ret = me->init_listen_sockfd();
	// assert(ret);

	//----------------------------------------------------------------


	int key;
	int uid;
	bool flag = true;

	// 当前是否已经登录
	bool if_login = false;

	// 当前是否已经开始监听 listen_sockfd
	bool if_listen = false;
	
	std::cout << "welcome to chat_geloutingyu~" << std::endl;

	while(flag) {
		if(!if_login) {
			// 如果当前用户登录了但是还没开始监听 listen_sockfd 则开始监听 listen_sockfd
			if(!if_listen) {
				if_listen = true;

				// 对 listen_sockfd 进行 listen 系统调用并
				// 将 listen_sockfd 注册到 epoll 内核事件表
				me->init_listen_sockfd();

				// 开始监听其它 client 发送的消息
				me->listen_msg();
			}

			std::cout << "-----------------------------------------------" << std::endl;
			std::cout << "you can input 2 to registered an account number" << std::endl;
			std::cout << "you can input 3 to login" << std::endl;
			std::cout << "you can input 14 to delete user" << std::endl;
			std::cout << "you can input -1 to quit" << std::endl;
			std::cout << "you can input 0 to get help" << std::endl;
			std::cout << "-----------------------------------------------" << std::endl << std::endl;
			std::cout << ">>>";

		} else {
			std::cout << "-----------------------------------------------" << std::endl;
			std::cout << "you can input 4 x to show the user list(其中 x 表查看第 x 页的用户列表，每页最多 20 个用户)" << std::endl;
			std::cout << "you can input 5 x to find if a user is online(其中 x 为要查找的 user_id)" << std::endl;
			std::cout << "you can input 6 x to send message to x(其中 x 为在线用户 user_id)" << std::endl;
			// std::cout << "you can input 8 x to show the group list(其中 x 表查看第 x 页的群组列表，每页最多 20 个群组)" << std::endl;
			// std::cout << "you can input 11 to create a group" << std::endl;
			std::cout << "you can input 12 to logout" << std::endl;
			std::cout << "you can input 0 to get help" << std::endl;
			std::cout << "-----------------------------------------------" << std::endl << std::endl;
			std::cout << ">>>";
		}

		std::cin >> key;

		// 登录前能进行的操作
		if(!if_login) {
			switch(key) {

				// 注册账号
				case 2: {
					std::string uname;
					std::string password;

					std::cout << "please input user name and password(user name and password are \
						no more than 16 bytes, consisting of letters and numbers)" << std::endl;
					std::cout << "user name:";

					std::cin >> uname;

					std::cout << "password:";

					std::cin >> password;

					// 如果检查 uanme 和 password 合法则进行账号注册操作
					if(check_uname(uname) && check_password(password)) {

						// 注册成功则返回注册的 uid (大于 0)，失败则返回 0
						uid = me->registered(uname, password);

						// 注册账号成功
						if(uid > 0) {
							std::cout << "registered success~" << std::endl;
							std::cout << "your user id is: " << uid << std::endl;

						} else {
							// 注册账号失败
							std::cout << "registered failed!! you can do it agian~" << std::endl;
						}
					}

					break;
				}

				// 账号登录
				case 3: {
					std::string password;

					std::cout << "please input your user id and password~" << std::endl;
					std::cout << "uid:";

					// 输入账号即 uid
					std::cin >> uid;

					std::cout << "password:";

					// 输入密码
					std::cin >> password;

					// 如果检查 password 合法则进行账号登录操作
					if(check_password(password)) {
						if(me->login(uid, password)) {
							if_login = true;

							std::cout << "login success~" << std::endl;
							std::cout << "then you can do this..." << std::endl;

						} else {
							std::cout << "login failed!! you can do it agian~" << std::endl;
						}
					}

					break;
				}

				case 14: {
					std::string password;

					std::cout << "please input your user id and password~" << std::endl;
					std::cout << "uid:";

					std::cin >> uid;

					std::cout << "password:";

					std::cin >> password;

					// 如果检查 password 合法则进行删除账号操作
					if(check_password(password)) {
						if(me->delete_user(uid, password)) {
							std::cout << "delete user " << uid << " success~" << std::endl;

						} else {
							std::cout << "delete user " << uid << " failed!! you can do it agian~" << std::endl;
						}
					}

					break;
				}

				case -1: {
					flag = false;

					break;
				}

				case 0: {
					break;
				}

				default: {
					std::cout << "you input an error key!!" << std::endl;

					break;
				}
			}

		} else {
			// 登录后能进行的操作
			switch(key) {

				// 列出在线用户列表
				case 4: {
					// 表查看第 page 页的用户列表
					int page;

					std::cin >> page;
					// std::cout << page << std::endl;

					std::cout << "the user list:" << std::endl;

					cout.setf(ios::left);
					std::cout << setw(19) << "user_id" << setw(19) << "user_name" << setw(19) << "online(1表在线,0不在线)" << std::endl;

					char *list = me->user_list(page);

					char *p = std::strtok(list, ",");

					while(p) {
						// std::cout << p << std::endl;

						// char *q = std::strtok(p, ":");
						// while(q) {
						// 	std::cout << q << std::endl;
						// 	q = std::strtok(NULL, ":");
						// }

						std::string tuple = p;
						int cnt = 0;
						for(int i = 0; i < tuple.size(); ++i) {
							if(tuple[i] == ':') {
								std::cout << setw(19) << tuple.substr(cnt, i - cnt);;
								cnt = i + 1;
							}
						}

						std::cout << setw(19) << tuple.substr(cnt, 1) << std::endl;

						p = std::strtok(NULL, ",");
					}

					break;
				}

				// 查找用户是否在线
				case 5: {
					// 要查找的 uid
					int uid;

					std::cin >> uid;

					// uid 在线
					if(me->check_user_online(uid)) {
						std::cout << "user: " << uid << " is online~" << std::endl;

					} else {
						// uid 不在线
						std::cout << "user: " << uid << " is not online~" << std::endl;
					}

					break;
				}

				// 发送消息给某用户
				case 6: {
					// 目标用户
					int uid;

					std::cin >> uid;

					if(me->check_user_online(uid) != true) {
						std::cout << "usre: " << uid << " is not online, can't send message to him~" << std::endl;
						break;
					}

					std::cout << "you can input quit to stop this session!!" << std::endl;

					// 初始化 client_b_sockfd 为指向 uid 的 socket 连接
					// 并且将 client_b_sockfd 注册到 epoll
					ret = me->init_client_b_sockfd(uid);
					assert(ret != -1);

					// 向 uid 发送信息
					me->send_msg_to_clien_b(uid);

					// 退出聊天后删除 client_b 的信息
					// 将 client_b_sockfd 从 epoll 移除并关闭 client_b_sockfd 连接
					// me->delete_client_b_sockfd();

					break;
				}

				// 展示群组列表
				case 8: {

					break;
				}

				// 创建一个群
				case 11: {
					// 创建的群组名称
					std::string gname;

					std::cout << "please input group name(user name and password are \
						no more than 16 bytes, consisting of letters and numbers):";

					std::cin >> gname;

					if(check_gname(gname)) {
						// 注册成功则返回注册的 uid (大于 0)，失败则返回 0
						int gid = me->create_group(gname);

						// 创建群组成功
						if(gid > 0) {
							std::cout << "create group success~" << std::endl;
							std::cout << "your group id is: " << gid << std::endl;

						} else {
							// 创建失败
							std::cout << "create group failed!! you can do it agian~" << std::endl;
						}
					}

					break;
				}

				// 退出登录
				case 12: {
					if(me->logout()) {
						if_login = false;

						// 退出登录后不再监听 listen_sockfd
						// 将 listen_sockfd 从 epoll 移除并关闭 listen_sockfd 连接
						// me->delete_listen_sockfd();

						// 将 epoll 内事件表上的其他 sockfd 移除避免再接收到其他用户的信息
						me->clean_epollfd();

						std::cout << "logout success~" << std::endl;
						std::cout << "then you can do this..." << std::endl;

					} else {
						std::cout << "logout failed!! you can do it agian~" << std::endl;
					}

					break;
				}

				case 0:{
					break;
				}

				default: {
					std::cout << "you input an error key!!" << std::endl;

					break;
				}
			}

		}

		
	}
	

	delete me;

	return 0;
}