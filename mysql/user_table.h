#ifndef USER_TABLE_H
#define USER_TABLE_H

#include <unistd.h>

#include "chat_geloutingyu_db.h"
#include "../config.h"


// sql 语句缓存空间
const int BUFF_SIZE = 300;
char buff[BUFF_SIZE];


class user_table {
public:
	user_table();
	~user_table();

	// 删除第 uid 条元组当 password 正确时
	static bool delete_where_uid_password(const int uid, const std::string password);


	// 执行带 where 的查询操作
	// column 表要查询的字段，用 ',' 分隔
	// where 用 , 分隔
	// 返回一个 mysqlpp::StoreQueryResult 对象的查询结果
	static mysqlpp::StoreQueryResult search_where(const std::string column = "*", const std::string where = "");

	// 执行带 limit 的查询操作
	// column 表要查询的字段，用 ',' 分隔
	// page 表页数，cap 表每页的容量, 其中 page 从 1 开始计数
	// 返回一个 mysqlpp::StoreQueryResult 对象的查询结果
	static mysqlpp::StoreQueryResult search_limit(const std::string column = "*", const int page = 1, const int cap = 20);

	// 执行插入操作
	// 插入成功则返回 uid (大于 0)，失败则返回 0
	static int insert(std::string uname, std::string password, std::string addr = "", std::string j_group = "", std::string c_group = "");

	// 执行更新操作， 当 uid 对应的 password 和数据库中一致时将数据库中 online 字段更新为 true，将数据看中 addr 字段更新为 addr 形参的值
	// uid 表用户账号，online 表用户是否在线，addr 为用户登录的地址(ip:port)，password 为用户密码
	// 成功返回 true，失败则返回 false
	static bool update_online_and_addr(int uid, int online, const std::string &password = "", const std::string &addr = "");

	// 更新 uid 对应元组的 c_group_num 和 c_group 字段
	// 成功返回 true, 失败返回 false
	static bool update_c_group_num_and_c_group(const int uid, const std::string &password, const int c_group_num, const std::string &c_group);

	// static mysqlpp::StoreQueryResult search(std::string sql);

private:
	// 获取数据库对象实例
	static chat_geloutingyu_db* get_db();
	
};


// 删除第 uid 条元组当 password 正确时
bool user_table::delete_where_uid_password(const int uid, const std::string password) {
	std::string delete_format = "delete from user where uid = %d && password = \"%s\";";

	snprintf(buff, sizeof(buff), delete_format.c_str(), uid, password.c_str());

	// 获取 chat_geloutingyu_db 类实例
	auto db = user_table::get_db();

	// 调用 chat_geloutingyu_db 中的数据库查询操作函数
	return db->delete_from(buff);
}


// 执行带 where 的查询操作
// column 表要查询的字段，用 ',' 分隔
// where 用 , 分隔
// 返回一个 mysqlpp::StoreQueryResult 对象的查询结果
mysqlpp::StoreQueryResult user_table::search_where(const std::string column, const std::string where) {
	std::string search_format = "select %s from user where %s;";

	snprintf(buff, sizeof(buff), search_format.c_str(), column.c_str(), where.c_str());

	// printf("%s\n", buff);

	// 获取 chat_geloutingyu_db 类实例
	auto db = user_table::get_db();

	// 调用 chat_geloutingyu_db 中的数据库查询操作函数
	return db->search(buff);
}


// 执行带 limit 的查询操作
// column 表要查询的字段，用 ',' 分隔
// page 表页数，cap 表每页的容量, 其中 page 从 1 开始计数
// 返回一个 mysqlpp::StoreQueryResult 对象的查询结果
mysqlpp::StoreQueryResult user_table::search_limit(const std::string column, const int page, const int cap) {
	std::string search_format = "select %s from user limit %d, %d;";

	// 从第 start + 1 行开始查询
	const int start = (page - 1) * cap;

	snprintf(buff, sizeof(buff), search_format.c_str(), column.c_str(), start, cap);

	// printf("%s\n", buff);

	// 获取 chat_geloutingyu_db 类实例
	auto db = user_table::get_db();

	// 调用 chat_geloutingyu_db 中的数据库查询操作函数
	return db->search(buff);
}


// 执行更新操作， 当 uid 对应的 password 和数据库中一致时将数据库中 online 字段更新为 true，将数据看中 addr 字段更新为 addr 形参的值
// uid 表用户账号，online 表用户是否在线，addr 为用户登录的地址(ip:port)，password 为用户密码
// 成功返回 true，失败则返回 false
bool user_table::update_online_and_addr(int uid, int online, const std::string &password, const std::string &addr) {
	std::string update_format = "update user set online = %d, addr = \"%s\", time = %d where uid = %d && password = \"%s\";";
	memset(buff, 0, BUFF_SIZE);
	time_t t = (int)time(NULL);
	int tim = time(&t);

	snprintf(buff, sizeof(buff), update_format.c_str(), online, addr.c_str(), tim, uid, password.c_str());

	// printf("%s\n", buff);

	// 获取 chat_geloutingyu_db 类实例
	auto db = user_table::get_db();

	// 调用 chat_geloutingyu_db 中的数据库插入操作函数
	return db->update(buff);
}


// 更新 uid 对应元组的 c_group_num 和 c_group 字段
// 成功返回 true, 失败返回 false
bool user_table::update_c_group_num_and_c_group(const int uid, const std::string &password, const int c_group_num, const std::string &c_group) {
	std::string update_format = "update user set c_group_num = %d, c_group = \"%s\", time = %d where uid = %d && password = \"%s\";";
	memset(buff, 0, BUFF_SIZE);
	time_t t = (int)time(NULL);
	int tim = time(&t);

	snprintf(buff, sizeof(buff), update_format.c_str(), c_group_num, c_group.c_str(), tim, uid, password.c_str());

	// printf("%s\n", buff);

	// 获取 chat_geloutingyu_db 类实例
	auto db = user_table::get_db();

	// 调用 chat_geloutingyu_db 中的数据库插入操作函数
	return db->update(buff);
}


// 执行插入操作
// 插入成功则返回 uid (大于 0)，失败则返回 0
int user_table::insert(std::string uname, std::string password, std::string addr, std::string j_group, std::string c_group) {
	std::string insert_format = "insert into user(uname, password, addr, j_group, c_group, time) values(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d);";
	memset(buff, 0, BUFF_SIZE);
	time_t t = (int)time(NULL);
	int tim = time(&t);

	snprintf(buff, sizeof(buff), insert_format.c_str(), uname.c_str(), password.c_str(), addr.c_str(), j_group.c_str(), c_group.c_str(), tim);

	// std::string sql = buff;

	// 获取 chat_geloutingyu_db 类实例
	auto db = user_table::get_db();

	// 调用 chat_geloutingyu_db 中的数据库插入操作函数
	return db->insert(buff);
}


// 获取数据库对象实例
chat_geloutingyu_db* user_table::get_db() {
	// db_name, db_ip, db_user, db_passwd, db_port, db_charset, max_size
	return chat_geloutingyu_db::create(config::get_db_name(), config::get_db_ip(), 
		config::get_db_user(), config::get_db_passwd(), config::get_db_port(), config::get_db_charset(), config::get_max_size());
}


// 默认构造函数实现
user_table::user_table() {

}


// 析构函数实现
user_table::~user_table() {

}


#endif