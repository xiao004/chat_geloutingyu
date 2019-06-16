#ifndef GROUP_TABLE_H
#define GROUP_TABLE_H

#include <unistd.h>

#include "chat_geloutingyu_db.h"
#include "../config.h"


// sql 语句缓存空间
const int GROUP_BUFF_SIZE = 300;
char group_buff[GROUP_BUFF_SIZE];


class group_table {
public:
	group_table();
	~group_table();

	// 执行插入操作
	// 第一个参数是创建者的 uid,第二个参数是创建的群组名称
	// 插入成功则返回 uid (大于 0)，失败则返回 0
	static int insert(const int uid, const std::string &gname);
	
private:
	// 获取数据库对象实例
	static chat_geloutingyu_db* get_db();
};


// 执行插入操作
	// 第一个参数是创建者的 uid,第二个参数是创建的群组名称
	// 插入成功则返回 uid (大于 0)，失败则返回 0
int group_table::insert(const int uid, const std::string &gname) {
	std::string insert_format = "insert into user_group(gname, c_id, users, time) values(\"%s\", %d, \"%d\", %d);";
	memset(group_buff, 0, GROUP_BUFF_SIZE);
	time_t t = (int)time(NULL);
	int tim = time(&t);

	snprintf(group_buff, sizeof(group_buff), insert_format.c_str(), gname.c_str(), uid, uid, tim);

	// std::string sql = group_buff;

	// 获取 chat_geloutingyu_db 类实例
	auto db = group_table::get_db();

	// 调用 chat_geloutingyu_db 中的数据库插入操作函数
	return db->insert(group_buff);
}


// 默认构造函数实现
group_table::group_table() {

}


// 默认析构函数
group_table::~group_table() {

}


// 获取数据库对象实例
chat_geloutingyu_db* group_table::get_db() {
	// db_name, db_ip, db_user, db_passwd, db_port, db_charset, max_size
	return chat_geloutingyu_db::create(config::get_db_name(), config::get_db_ip(), 
		config::get_db_user(), config::get_db_passwd(), config::get_db_port(), config::get_db_charset(), config::get_max_size());
}


#endif