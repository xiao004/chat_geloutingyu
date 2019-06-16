#include <unistd.h>
#include <iostream>

#include "route_server.h"
#include "route_processpool.h"
#include "../internal/work_server_addr.h"


int main(int argc, char const *argv[]) {

	if(argc <= 2) {
		printf("usage: %s, route_server ip, route_server port\n", basename(argv[0]));
		return 1;
	}

	const char *ip = argv[1];
	int port = atoi(argv[2]);

	printf("%s %d\n", ip, port);

	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);

	int ret = 0;
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(listenfd, 5);
	assert(ret != -1);

	// 获取当前机器中可用的 cpu 数目
	int cpus = (int)sysconf(_SC_NPROCESSORS_ONLN);
	std::cout << "初始化 route server 进程数为: " << cpus << std::endl;

	// 创建进程池实列
	// 第一个参数为监听 socket，第二个参数为进程池中初始化的进程数
	route_processpool<route_server> *pool = route_processpool<route_server>::create(listenfd, cpus);

	if(pool) {
		pool->run();
		delete pool;
	}

	close(listenfd);
	
	return 0;
}