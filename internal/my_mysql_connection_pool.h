#ifndef MY_MYSQL_CONNECTION_POOL_H
#define MY_MYSQL_CONNECTION_POOL_H

#include <mysql++/mysql++.h>
#include <string>
#include <unistd.h>

/*
my_mysql_connection_pool 继承自 mysqlpp::Connection 类，需要实现该类中的纯虚函数
my_mysql_connection_pool 的对象用于初始化 mysqlpp::ScopedConnection 类对象:
virtual Connection* create() = 0;
virtual void destroy(Connection*) = 0;
virtual unsigned int max_idle_time() = 0;

然后可以通过 mysqlpp::ScopedConnection 类对象来自动获取 mysql 连接
*/

class my_mysql_connection_pool : public mysqlpp::ConnectionPool {

public:
	my_mysql_connection_pool(std::string dbname, std::string serverip, std::string user, std::string passwd, int port, std::string charset, int max_size) :
		m_dbname(dbname), m_server_ip(serverip), m_user(user), m_password(passwd), m_charset(charset), m_port(port) {

		m_max_size = max_size;
		// 初始化时使用的连接数为 0
		conns_in_use_ = 0;
		// 设置操作超时时间为 300ms
		m_max_idle_time = 300;
	}

	// 析构函数，防止实现多态时无法释放基类资源(非虚函数的析构函数只负责释放本类对象的资源)，将其定义为虚函数
	virtual ~my_mysql_connection_pool() {
		std::cout << "~my_mysql_connection_pool" << std::endl;
		// 调用继承自父类中的 clear 成员，释放堆内存
		clear();
	}

	// 获取当连接池中的连接数
	int get_size() {
		return mysqlpp::ConnectionPool::size();
	}

	// 获取数据库名
	const std::string& get_dbname() const {
		return m_dbname;
	}

	// 获取 mysql 服务主机 ip
	const std::string& get_server_ip() const {
		return m_server_ip;
	}

	// 获取用户名
	const std::string& get_user() const {
		return m_user;
	}

	// 获取用户密码
	const std::string& get_password() const {
		return m_password;
	}

	// 获取编码格式
	const std::string& get_charset() const {
		return m_charset;
	}

	// 获取正在使用的连接数
	const int get_conn_in_use() const {
		return conns_in_use_;
	}

	// 获取 mysql 服务端口
	const int get_port() const {
		return m_port;
	}

	// 设置超时时间
	void set_max_idle_time(int max_idle) {
		m_max_idle_time = max_idle;
	}

	// 从连接池获取一个连接，如果没有创建一个
	virtual mysqlpp::Connection* grab() {
		// 连接数达到上限
		while(conns_in_use_ > m_max_size) {
			std::cout << "wait conn release" << std::endl;
			sleep(1);
		}
		++conns_in_use_;
		return mysqlpp::ConnectionPool::grab();
	}

	// 从连接池中删除指定连接 pc
	void remove(const mysqlpp::Connection *pc) {
		mysqlpp::ConnectionPool::remove(pc);
	}

	// 将连接 pc 置为未使用状态
	virtual void release(const mysqlpp::Connection *pc) {
		--conns_in_use_;
		mysqlpp::ConnectionPool::release(pc);
	}


protected:
	// 创建一个连接，这个函数是在 mysqlpp::ScopedConnection 类中调用的 
	virtual mysqlpp::Connection *create() {
		mysqlpp::Connection *conn = new mysqlpp::Connection(true);
		mysqlpp::SetCharsetNameOption *popt = new mysqlpp::SetCharsetNameOption(m_charset.c_str());

		// 设置编码格式
		conn->set_option(popt);

		conn->connect(m_dbname.empty() ? 0 : m_dbname.c_str(),
						m_server_ip.empty() ? 0 : m_server_ip.c_str(),
						m_user.empty() ? 0 : m_user.c_str(),
						m_password.empty() ? "" : m_password.c_str(),
						m_port);

		std::cout << "create" << std::endl;
		return conn;
	}

	// 释放连接 cp 的资源，这个函数是在 mysqlpp::ScopedConnection 类中调用的 
	virtual void destroy(mysqlpp::Connection *cp) {
		std::cout << "destroy" << std::endl;
		delete cp;
	}

	// 获取当前超时时间，这个函数是在 mysqlpp::ScopedConnection 类中调用的 
	virtual unsigned int max_idle_time() {
		return m_max_idle_time;
	}

	
private:
	// 要操作的数据库名称
	std::string m_dbname;

	// mysql 服务所在的主机 ip
	std::string m_server_ip;

	// 登录的 mysql 用户
	std::string m_user;

	// 登录密码
	std::string m_password;

	// 编码格式
	std::string m_charset;

	// mysql 进程通信的端口
	int m_port;

	// 操作超时时间
	int m_max_idle_time;

	// 当前正在使用的连接数
	unsigned int conns_in_use_;

	// 最多能开启的连接数即连接池的大小
	int m_max_size;
};

#endif