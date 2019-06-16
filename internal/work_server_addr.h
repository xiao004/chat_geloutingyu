// 表示服务器地址及对应服务器当前用户数的结构体

#ifndef WORK_SERVER_ADDR
#define WORK_SERVER_ADDR


struct address {
	char ip[17]; // 对应服务器的 ip
	int port; // 对应服务器的 port
};


// 表示服务器地址及对应服务器当前用户数的结构体
struct work_server_addr {
	address addr; // 对应服务器的 ip + port
	int connections; // 对应服务器当前用户连接数
};



#endif