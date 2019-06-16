#include "message.h"


// 往 sockfd 发送类型为 type 内容为 buf 的报文实现
int send_msg(const int sockfd, const int type, const char *buf) {

	struct message msg;
	msg.head.type = type;
	msg.head.length = MSG_HEAND;

	if(buf != NULL) {
		memcpy(&msg.body, buf, strlen(buf) + 1);

		msg.head.length += strlen(buf) + 1;
	}

	return send(sockfd, (char*)&msg, msg.head.length, 0);

	// printf("%s\n", msg.body);
	// printf("%d\n", msg.head.length);
}