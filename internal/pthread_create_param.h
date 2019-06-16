
#ifndef PTHREAD_CREATE_PARAM_H
#define PTHREAD_CREATE_PARAM_H

#include "work_server_addr.h"


struct pthread_create_param {
	// 发送心跳包的 socket
	int connectfd;

	// route server 地址
	address addr;
};


#endif