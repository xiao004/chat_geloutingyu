#include <cstring>
#include <string>

#include "work_server.h"
#include "work_processpool.h"
#include "../internal/message.h"
#include "../mysql/user_table.h"
#include "../mysql/group_table.h"


// 类外初始化静态成员变量
int work_server::m_epollfd = -1;


// 打印客户端信息
void print_sockaddr_in(struct sockaddr_in *address) {
	printf("==========================================================\r\n");
    printf("a new socket client add, the infomation of this client is:\n");
    printf("sin_addr : %s\n", inet_ntoa(address->sin_addr));
    printf("sin_port : %d\n", ntohs(address->sin_port));
    printf("sin_family : %d\n", address->sin_family);
    printf("==========================================================\r\n");
}


void print_share_mem(int *share_mem) {
	printf("+++++++++++++++++++++++\r\n");
	printf("%d\n", *share_mem);
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


// 有新连接加入 & work_server 类初始化成员函数实现
void work_server::init(int epollfd, int sockfd, const sockaddr_in& client_addr, int *share_mem_, int sem_id_) {
	m_epollfd = epollfd;
	m_sockfd = sockfd;
	m_address = client_addr;

	// 初始化 m_uid 和 m_password
	m_uid = 0;
	m_password = "";

	// 初始化共享内存地址 
	share_mem = share_mem_;

	// 初始化信号量集标识 id
	sem_id = sem_id_;

	memset(m_buf, '\0', BUFFER_SIZE);

	m_read_idx = 0;

	// 当前机器连接数加一
	share_mem_increase();
}


// work_server 业务逻辑成员函数实现
void work_server::process() {
	int idx = 0;
	int ret = -1;

	while(true) {
		idx = m_read_idx;

		// memset(m_buf, '\0', BUFFER_SIZE);
		// ret = recv(m_sockfd, m_buf + idx, BUFFER_SIZE - 1 - idx, 0);
		ret = recv(m_sockfd, m_buf, BUFFER_SIZE - 1, 0);
		// printf("%d\n", ret);

		// 如果读操作发生错误，则关闭客户连接．但是如果是暂时无数据可读，则退出循环
		if(ret < 0) {
			// EAGAIN 表连接没有完全建立但正在建立中．通常发生在客户端发起异步 connect 的场景中
			// 对于这种情况直接 break，等待下一次事件触发即可
			// 对于其他错误情况则断开连接
			if(errno != EAGAIN) {
				removefd(m_epollfd, m_sockfd);

				// client 端连接断开，则用户退出登录
				memset(m_buf, '\0', sizeof(m_buf));
				sprintf(m_buf, "%d:%s", m_uid, m_password.c_str());
				logout(m_buf);

				// 当前机器的连接数减一
				share_mem_reduce();
			}
			break;

		} else if(ret == 0) {
			removefd(m_epollfd, m_sockfd);

			// client 端连接断开，则用户退出登录
			memset(m_buf, '\0', sizeof(m_buf));
			sprintf(m_buf, "%d:%s", m_uid, m_password.c_str());
			logout(m_buf);

			// 当前机器的连接数减一
			share_mem_reduce();

			break;

		} else {
			m_buf[ret] = '\0';

			struct message *msg = (message*)m_buf;

			print_msg(msg, &m_address);

			switch((msg->head).type) {

				// 注册账号请求
				case REGISRERED: {
					char uid_str[10];
					int uid = registered(msg->body);
					
					// 注册成功则返回一个 type 为 REGISRERED 内容为 uid (其值大于 0) 的报文
					if(uid > 0) {
						sprintf(uid_str, "%d", uid);
						send_msg(m_sockfd, REGISRERED, uid_str);

					} else {
						// 注册失败则返回一个 type 为 REGISRERED 内容为 0 的报文
						uid = 0;
						sprintf(uid_str, "%d", uid);
						send_msg(m_sockfd, REGISRERED, uid_str);
					}

					break;
				}

				// 登录账号请求
				case LOGIN: {

					char addr[25];

					sprintf(addr, "%s:%d", inet_ntoa(m_address.sin_addr), ntohs(m_address.sin_port));

					// printf("%s\n", addr);

					// 登录成功标志
					char flag[2];
					
					// 登录成功
					if(login(msg->body, addr)) {
						sprintf(flag, "%d", 1);

					} else {
						// 登录失败
						sprintf(flag, "%d", 0);
					}

					send_msg(m_sockfd, LOGIN, flag);

					break;
				}

				// 退出登录请求
				case LOGOUT: {
					// 退出登录成功标志
					char flag[2];
					
					// 退出登录成功
					if(logout(msg->body)) {
						sprintf(flag, "%d", 1);

					} else {
						// 退出登录失败
						sprintf(flag, "%d", 0);
					}

					send_msg(m_sockfd, LOGOUT, flag);

					break;	
				}

				// 展示用户列表请求
				case USER_LIST: {

					// 获取在线用户列表
					const char *list = user_list(msg->body);

					// 获取列表成功
					if(list) {
						// printf("%s\n", list);
						send_msg(m_sockfd, USER_LIST, m_buf);

					} else {
						// 获取列表失败
						send_msg(m_sockfd, USER_LIST, "");
					}

					break;
				}

				// 判断指定用户是否在线请求
				case FIND_USER: {
					// 判断用户是否在线标志
					char flag[2];

					// 判断的用户在线
					if(check_user_online(msg->body)) {
						sprintf(flag, "%d", 1);

					} else {
						// 判断的用户不在线
						sprintf(flag, "%d", 0);
					}

					send_msg(m_sockfd, FIND_USER, flag);

					break;
				}

				// 获取指定用户的登录地址
				case OBTAIN_CLIENT_ADDR: {

					// 获取用户登录地址
					const char *addr = obtain_client_addr(msg->body);

					// 获取用户登录地址成功
					if(addr) {
						send_msg(m_sockfd, OBTAIN_CLIENT_ADDR, addr);

					} else {
						// 获取用户登录地址失败
						send_msg(m_sockfd, OBTAIN_CLIENT_ADDR, "");
					}

					break;
				}

				// 删除账号
				case DELETE_USER: {
					// 删除成功标志
					char flag[2];
					
					// 退出登录成功
					if(delete_user(msg->body)) {
						sprintf(flag, "%d", 1);

					} else {
						// 退出登录失败
						sprintf(flag, "%d", 0);
					}

					send_msg(m_sockfd, DELETE_USER, flag);

					break;
				}

				// 创建群组
				case CREATE_GROUP: {
					char gid_str[10];
					int gid = create_group(msg->body);
					
					// 返回一个type为CREATE_GROUP,内容为uid_str的报文
					// 创建成功时 uid_str 为 gid(大于0),失败时为 0
					sprintf(gid_str, "%d", gid);
					send_msg(m_sockfd, CREATE_GROUP, gid_str);

					break;
				}

				default: {
					break;
				}
			}

		}

	}
}


// 创建群组
// 创建成功则返回注册的 gid (大于 0)，失败则返回 0
int work_server::create_group(const std::string &uid_password_gname) {
	auto idex = uid_password_gname.find(':');

	// uid_password_gname 格式有误，没有':'分隔账号和密码
	if(idex == std::string::npos) {
		std::cout << "the format of uid_password_gname error!" << std::endl;
		return 0;
	}

	// 执行操作的用户 uid
	int uid = atoi((uid_password_gname.substr(0, idex)).c_str());

	auto idex_2 = uid_password_gname.find(':', idex + 1);

	// uid_password_gname 格式有误，没有':'分隔账号密码和gname
	if(idex_2 == std::string::npos) {
		std::cout << "the format of uid_password_gname error!" << std::endl;
		return 0;
	}

	// 执行操作的用户 uid 对应的密码
	std::string password = uid_password_gname.substr(idex + 1, idex_2 - idex - 1);

	// 要创建的群组名
	std::string gname = uid_password_gname.substr(idex_2 + 1);

	// 如果执行操作的 uid 已经登录则继续执行操作
	if(is_login(uid, password)) {
		std::string where = "uid=%d";
		snprintf(m_buf, sizeof(m_buf), where.c_str(), uid);

		// 获取 uid 对应元组中的 c_group_num, c_group_num_max, c_group
		mysqlpp::StoreQueryResult res = user_table::search_where("c_group_num,c_group_num_max,c_group", m_buf);

		// 用户 uid 当前创建的群组数
		int c_group_num = res[0][0];

		// 用户 uid 最多能创建的群组数
		int c_group_num_max = res[0][1];

		// 用户 uid 创建的群组 gid 列表
		std::string c_group = (std::string)res[0][2];

		// 用户 uid 创建的群组数没有达到上限,可以创建群组
		if(c_group_num < c_group_num_max) {
			// 创建的群组 id
			int gid = group_table::insert(uid, gname);

			snprintf(m_buf, sizeof(m_buf), "%d", gid);

			std::string gid_str = m_buf;

			if(c_group == "") {
				c_group = gid_str;

			} else {
				c_group += ":";
				c_group += gid_str;
			}

			int cnt = user_table::update_c_group_num_and_c_group(uid, password, c_group_num + 1, c_group);

			if(cnt > 0) {
				return gid;

			} else {
				return 0;
			}

		} else {
			return 0;
		}
	}

	return 0;
}


// 删除账号
// 删除成功返回 true, 失败则返回 false
bool work_server::delete_user(const std::string uid_password) {
	auto idex = uid_password.find(':');

	// uid_password 格式有误，没有':'分隔账号和密码
	if(idex == std::string::npos) {
		std::cout << "the format of uid_password error!" << std::endl;
		return 0;
	}

	std::string uid_str = uid_password.substr(0, idex);
	int uid = atoi(uid_str.c_str());
	std::string password = uid_password.substr(idex + 1);

	return user_table::delete_where_uid_password(uid, password);
}


// 获取 client 的登录地址并返回该结果
const char* work_server::obtain_client_addr(const std::string &uid_password_client) {
	auto idex = uid_password_client.find(':');

	// uid_password_client 格式有误，没有':'分隔账号和密码
	if(idex == std::string::npos) {
		std::cout << "the format of uid_password_client error!" << std::endl;
		return NULL;
	}

	// 执行操作的用户 uid
	int uid = atoi((uid_password_client.substr(0, idex)).c_str());

	auto idex_2 = uid_password_client.find(':', idex + 1);

	// uid_password_client 格式有误，没有':'分隔账号密码和client
	if(idex_2 == std::string::npos) {
		std::cout << "the format of uid_password_client error!" << std::endl;
		return NULL;
	}

	// 执行操作的用户 uid 对应的密码
	std::string password = uid_password_client.substr(idex + 1, idex_2 - idex - 1);

	int client = atoi((uid_password_client.substr(idex_2 + 1)).c_str());

	// std::cout << uid << " " << password << " " << client << std::endl;

	std::string where = "uid=%d";
	snprintf(m_buf, sizeof(m_buf), where.c_str(), client);

	// std::cout << m_buf << std::endl;

	// 如果执行操作的 uid 已经登录则继续执行操作
	if(is_login(uid, password)) {
		mysqlpp::StoreQueryResult res = user_table::search_where("addr", m_buf);

		if(!res || res.num_rows() <= 0) {
			return NULL;
		}

		// std::cout << res[0][0] << std::endl;

		memcpy(m_buf, res[0][0], strlen(res[0][0]) + 1);

		return m_buf;
	}

	return NULL;
}


// 检查 uid 为 who 的用户是否在线
// 在线返回 true，否则返回 false
bool work_server::check_user_online(const std::string &uid_password_who) {
	auto idex = uid_password_who.find(':');

	// uid_password_who 格式有误，没有':'分隔账号和密码
	if(idex == std::string::npos) {
		std::cout << "the format of uid_password_who error!" << std::endl;
		return false;
	}

	// 执行操作的用户 uid
	int uid = atoi((uid_password_who.substr(0, idex)).c_str());

	auto idex_2 = uid_password_who.find(':', idex + 1);

	// uid_password_who 格式有误，没有':'分隔账号密码和who
	if(idex_2 == std::string::npos) {
		std::cout << "the format of uid_password_who error!" << std::endl;
		return false;
	}

	// 执行操作的用户 uid 对应的密码
	std::string password = uid_password_who.substr(idex + 1, idex_2 - idex - 1);

	int who = atoi((uid_password_who.substr(idex_2 + 1)).c_str());

	// std::cout << uid << " " << password << " " << who << std::endl;

	std::string where = "uid=%d";
	snprintf(m_buf, sizeof(m_buf), where.c_str(), who);

	// std::cout << m_buf << std::endl;

	// 如果执行操作的 uid 已经登录则继续执行操作
	if(is_login(uid, password)) {
		mysqlpp::StoreQueryResult res = user_table::search_where("online", m_buf);

		if(!res || res.num_rows() <= 0) {
			return false;
		}

		// std::cout << res[0][0] << std::endl;

		return atoi(res[0][0]);
	}

	return false;
}


// 展示用户列表
// 返回查找到的用户列表
const char* work_server::user_list(const std::string &page_cap) {
	auto idex = page_cap.find(':');

	// page_cap 格式有误，没有':'分隔账号和密码
	if(idex == std::string::npos) {
		std::cout << "the format of page_cap error!" << std::endl;
		return NULL;
	}

	// 获取页数
	const int page = atoi(page_cap.substr(0, idex).c_str());

	// 获取页面容量
	const int cap = atoi(page_cap.substr(idex+1).c_str());

	// 调用 user_table 的查询函数
	mysqlpp::StoreQueryResult ares = user_table::search_limit("uid,uname,online", page, cap);

	if(ares && ares.num_rows() > 0) {
		int idex = 0;
		// std::cout << "ares.num_rows() = " << ares.num_rows() << std::endl;

		for(size_t i = 0; i < ares.num_rows(); ++i) {
			mysqlpp::Row row = ares[i];
			char cnt[30];
			auto it = row.begin();

			sprintf(cnt, "%s:%s:%s,", (*(it)).c_str(), (*(it + 1)).c_str(), (*(it + 2)).c_str());

			memcpy(m_buf + idex, cnt, strlen(cnt) + 1);

			idex += strlen(cnt);

			// printf("%s\n", cnt);

			// for(auto it = row.begin(); it != row.end(); ++it) {
			// 	std::cout << *it << " ";
			// }
			// std::cout << std::endl;
		}

		// printf("%s\n", m_buf);
		return m_buf;
	}

	return NULL;
}


// 退出登录
// 退出登录成功返回 true, 失败则返回 false
bool work_server::logout(const std::string &uid_password) {
	auto idex = uid_password.find(':');

	// uid_password 格式有误，没有':'分隔账号和密码
	if(idex == std::string::npos) {
		std::cout << "the format of uid_password error!" << std::endl;
		return 0;
	}

	std::string uid_str = uid_password.substr(0, idex);
	int uid = atoi(uid_str.c_str());

	bool ret = user_table::update_online_and_addr(uid, 0, uid_password.substr(idex + 1));

	// 退出登录成功
	if(ret) {
		m_uid = 0;
		m_password = "";
	}

	return ret;
}


// 登录账号
// 登录成功返回 true，失败则返回 false
bool work_server::login(const std::string &uid_password, const std::string &addr) {
	auto idex = uid_password.find(':');

	// uid_password 格式有误，没有':'分隔账号和密码
	if(idex == std::string::npos) {
		std::cout << "the format of uid_password error!" << std::endl;
		return 0;
	}

	std::string uid_str = uid_password.substr(0, idex);
	int uid = atoi(uid_str.c_str());
	std::string password = uid_password.substr(idex + 1);

	bool ret = user_table::update_online_and_addr(uid, 1, password, addr);

	// 如果登录操作成功，则记录 uid 和 password
	if(ret) {
		m_uid = uid;
		m_password = password;
	}

	return ret;
}


// 注册账号
// 注册成功则返回注册的 uid (大于 0)，失败则返回 0
int work_server::registered(std::string uname_password) {
	auto idex = uname_password.find(':');

	// uname_password 格式有误，没有':'分隔账号和密码
	if(idex == std::string::npos) {
		std::cout << "the format of uname_password error!" << std::endl;
		return 0;
	}

	return user_table::insert(uname_password.substr(0, idex), uname_password.substr(idex + 1));
	// return user_table::insert("xiao", "123");
}


// 判断用执行操作的户是否登录
// 登录了返回 true, 否则返回 false
bool work_server::is_login(const int uid, const std::string &password) {
	// 如果当前接收到的 uid, password 和 login 操作时记录的一致则认为用户已经登录
	if(uid != 0 && uid == m_uid && password == m_password) {
		return true;
	}

	return false;
}


// op 为 -1 时执行 p 操作, op 为 1 时执行 v 操作
void work_server::pv(int sem_id, int op) {
	struct sembuf sem_b;

	sem_b.sem_num = 0; //信号量集中信号的标识符
	sem_b.sem_op = op;
	sem_b.sem_flg = SEM_UNDO; //当进程退出后取消正在进行的 semop 操作

	semop(sem_id, &sem_b, 1);
}


// 连接计数加一
void work_server::share_mem_increase() {
	pv(sem_id, -1);
	*share_mem += 1;
	pv(sem_id, 1);

	printf("a new client add, the new client addr is:\n");
	print_sockaddr_in(&m_address);
	printf("now the number of client in this server is:\n");
	print_share_mem(share_mem);
}


// 连接计数减一
void work_server::share_mem_reduce() {
	pv(sem_id, -1);
	*share_mem -= 1;
	pv(sem_id, 1);

	printf("a new client leave, this client addr is:\n");
	print_sockaddr_in(&m_address);
	printf("now the number of client in this server is:\n");
	print_share_mem(share_mem);
}