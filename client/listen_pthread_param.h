#ifndef LISTEN_PTHREAD_PARAM_H
#define LISTEN_PTHREAD_PARAM_H


// client::listen_pthread 函数的实参类型
struct listen_pthread_param {
	// epoll 内核表文件描述符
	int epollfd;

	// 监听 sockfd
	int listen_sockfd;
	
	// client_b_sockfd
	int client_b_sockfd;
};


#endif