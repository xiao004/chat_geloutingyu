#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include "work_server.h"
#include "work_processpool.h"
#include "../internal/work_server_addr.h"

#define REPORT_CONNECT_TIME_INTERVAL 10


int main(int argc, char const *argv[]) {

	if(argc <= 4) {
		printf("usage: %s, work_server ip, work_server port, route_server ip, route_server port\n", basename(argv[0]));
		return 1;
	}

	const char *ip = argv[1];
	int port = atoi(argv[2]);

	// 对外提供服务的 addr
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int ret = 0;

	// 监听 socket
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);

	// 向 route server 发送存活信息 & tcp 内网穿透的 socket
	int connectfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(connectfd);

	// 两个 socket 都要设置端口复用
	int opt = 1;
	ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	assert(ret != -1);

	ret = setsockopt(connectfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	assert(ret != -1);

	// 在 listen 之前对两个 socket 进行 bind
	ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = bind(connectfd, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(listenfd, 5);
	assert(ret != -1);

	// route server 的地址
	struct address route_addr;
	strcpy(route_addr.ip, argv[3]);
	route_addr.port = atoi(argv[4]);

	// struct sockaddr_in connect_addr;
	// bzero(&connect_addr, sizeof(connect_addr));
	// connect_addr.sin_family = AF_INET;
	// inet_pton(AF_INET, route_addr.ip, &connect_addr.sin_addr);
	// connect_addr.sin_port = htons(route_addr.port);

	// if(connect(connectfd, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) < 0) {
	// 	printf("connection failed\n");
	// 	close(connectfd);
	// }

	// while(true) {

	// }

	// 获取当前机器中可用的 cpu 数目
	int cpus = (int)sysconf(_SC_NPROCESSORS_ONLN);
	std::cout << "初始化 work server 进程数为: " << cpus << std::endl;

	// 创建进程池实列
	// 第一个参数为监听 socket，第二个参数为向 route server 发送存活信息 & tcp 内网穿透的 socket，它们绑定的同一个端口
	// 第三个参数为 route server 的地址，第四个参数为进程池中初始化的进程数
	work_processpool<work_server> *pool = work_processpool<work_server>::create(listenfd, connectfd, route_addr, cpus);

	if(pool) {
		pool->run();
		delete pool;
	}

	close(listenfd);
	close(connectfd);
	
	return 0;
}