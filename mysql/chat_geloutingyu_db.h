#ifndef CHAT_GELOUTINGYU_DB_H
#define CHAT_GELOUTINGYU_DB_H


#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <memory>

#include "../internal/my_mysql_connection_pool.h"
#include "../config.h"


// std::shared_ptr<my_mysql_connection_pool> db_pool = nullptr;
// std::string db_name = "chat_geloutingyu";
// std::string db_ip = "localhost";
// std::string db_user = "root";
// std::string db_passwd = "mysql";
// int db_port = 3306;
// std::string db_charset = "utf8";
// int max_size = 10;
// int BUFF_SIZE = 1024;
// char buff[1024];


// 操作 chat_geloutingyu 数据库的类
class chat_geloutingyu_db {
private:
	chat_geloutingyu_db(std::string db_name, std::string db_ip, std::string db_user, std::string db_passwd, int db_port, std::string db_charset, int max_size);
	

public:
	// 执行插入操作 sql 语句
	// 插入成功则返回 uid (大于 0)，失败则返回 0
	int insert(std::string sql);

	// 执行查询操作 sql 语句
	mysqlpp::StoreQueryResult search(std::string sql);

	// 执行更新操作 sql 语句
	bool update(std::string sql);

	// 执行删除操作 sql 语句
	bool delete_from(std::string sql);

	~chat_geloutingyu_db();

	// 获取本类对象单例函数
	static chat_geloutingyu_db* create(std::string db_name, std::string db_ip, std::string db_user, std::string db_passwd, int db_port, std::string db_charset, int max_size);

private:
	// // 要操作的数据库名称
	// std::string m_dbname;

	// // mysql 服务所在的主机 ip
	// std::string m_server_ip;

	// // 登录的 mysql 用户
	// std::string m_user;

	// // 登录密码
	// std::string m_password;

	// // 编码格式
	// std::string m_charset;

	// // mysql 进程通信的端口
	// int m_port;

	// // 操作超时时间
	// int m_max_idle_time;

	// // 最多能开启的连接数即连接池的大小
	// int m_max_size;

	// mysql 连接池
	std::shared_ptr<my_mysql_connection_pool> db_pool;

	// 单例对象
	static chat_geloutingyu_db *m_instance;
	
};

// 类外定义并初始化静态成员变量
chat_geloutingyu_db* chat_geloutingyu_db::m_instance = NULL;


// 执行删除操作 sql 语句
bool chat_geloutingyu_db::delete_from(std::string sql) {
	mysqlpp::ScopedConnection conn(*db_pool, false);
	mysqlpp::Query query = conn->query();
	query << sql;

	mysqlpp::SimpleResult res = query.execute();

	// query 影响的行数
	int rows = res.rows();

	if(res && rows > 0) {
		std::cout << "delete success~" << std::endl;
		return true;
	}

	std::cout << "delete failed!" << std::endl;
	return false;
}


// 执行更新操作 sql 语句
bool chat_geloutingyu_db::update(std::string sql) {
	mysqlpp::ScopedConnection conn(*db_pool, false);
	mysqlpp::Query query = conn->query();
	query << sql;

	mysqlpp::SimpleResult res = query.execute();

	// update 影响的行数
	int rows = res.rows();

	// std::cout << rows << std::endl;

	if(rows > 0) {
		std::cout << "update success!" << std::endl;
		return true;
	}

	std::cout << "update failed!" << std::endl;
	return false;
}


// 执行查询 sql 语句
mysqlpp::StoreQueryResult chat_geloutingyu_db::search(std::string sql) {
	mysqlpp::ScopedConnection conn(*db_pool, false);
	mysqlpp::Query query = conn->query();
	query << sql;

	mysqlpp::StoreQueryResult ares = query.store();
	
	// 打印数据
	// if(ares) {
	// 	std::cout << "ares.num_rows() = " << ares.num_rows() << std::endl;

	// 	for(size_t i = 0; i < ares.num_rows(); ++i) {
	// 		// std::cout << "uid: " << ares[i]["uid"] << "\tuname: " << ares[i]["uname"] << "\tpassword: " << ares[i]["password"] << "\tonline: " << ares[i]["online"] << std::endl;
	// 		mysqlpp::Row row = ares[i];
	// 		// cout << row[0] << " " << row[1] << endl;
	// 		for(auto it = row.begin(); it != row.end(); ++it) {
	// 			std::cout << *it << " ";
	// 		}
	// 		std::cout << std::endl;
	// 	}
	// }

	return ares;
}


// 执行插入操作 sql 语句
// 插入成功则返回 uid (大于 0)，失败则返回 0
int chat_geloutingyu_db::insert(std::string sql) {
	mysqlpp::ScopedConnection conn(*db_pool, false);
	mysqlpp::Query query = conn->query();
	query << sql;

	mysqlpp::SimpleResult res = query.execute();

	if(res) {
		std::cout << "inserted seccess~, insert_id = " << res.insert_id() << std::endl;	
		return res.insert_id();
	}

	std::cout << "inserted failed!" << std::endl;
	return 0;
}


// 构造函数实现
chat_geloutingyu_db::chat_geloutingyu_db(std::string db_name, std::string db_ip, std::string db_user, std::string db_passwd, int db_port, std::string db_charset, int max_size) {

	db_pool = std::make_shared<my_mysql_connection_pool>(db_name, db_ip, db_user, db_passwd, db_port, db_charset, max_size);
}


// 析构函数实现
chat_geloutingyu_db::~chat_geloutingyu_db() {

}


// 获取单例对象函数实现
chat_geloutingyu_db* chat_geloutingyu_db::create(std::string db_name, std::string db_ip, std::string db_user, std::string db_passwd, int db_port, std::string db_charset, int max_size) {
	if(!m_instance) {
		m_instance = new chat_geloutingyu_db(db_name, db_ip, db_user, db_passwd, db_port, db_charset, max_size);
	}

	return m_instance;
}


#endif