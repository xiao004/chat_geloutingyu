// 应用层协议数据结构定义

#ifndef MESSAGE_H
#define MESSAGE_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

// 首部数据结构
#define MSG_TYPE_LEN 2
#define MSG_LENGTH_LEN 2
#define MSG_HEAND (MSG_LENGTH_LEN + MSG_TYPE_LEN)

/* a message is a tcp datagram with following structure:
   --+---16bits--+-----16bits----------+---len*8bits---+
   --+ msg type  + msg length(exclude) + message body  +
   --+-----------+---------------------+---------------+
*/


// msg type 值及其对应的宏
#define OBTAIN_SERVER_ADDR 0	//请求获取ip+port /回应
#define REPORT_CONNECT 1		//work server向route server 汇报当前tcp连接数
#define REGISRERED 2			//注册账号请求／回应 uname:password
#define LOGIN 3					//登录请求／回应
#define USER_LIST 4 			//列出当前在线用户列表／回应
#define FIND_USER 5				//查找xxx是否在线／回应
#define OBTAIN_CLIENT_ADDR 6	//向work server 请求获取xxx用户的ip:port
#define USER_CONVERSATION 7		//表用户之间的会话内容
#define GROUP_LIST 8			//列出群列表／回应
#define JOIN_GROUP 9			//加入群xxx／回应
#define GROUP_CONVERSATION 10	//在xxx群发送消息
#define CREATE_GROUP 11			//创建一个群
#define LOGOUT 12				//退出登录
#define ERROR 13				//错误报文
#define DELETE_USER 14			//删除账号


// 报文定长头部
struct message_head {
	uint16_t type; //报文类型
	uint16_t length; //报文总长度
};

// 报文结构体
struct message {
	// 消息体容量
	static const int BUFFER_SIZE = 1024;

	// 首部
	message_head head;
	
	// 消息体
	// const char *body;
	char body[BUFFER_SIZE];
};


// 往 sockfd 发送类型为 type 内容为 buf 的报文
int send_msg(const int sockfd, const int type, const char *buf);


#endif