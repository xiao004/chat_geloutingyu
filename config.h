#ifndef CONFIG_H
#define CONFIG_H


class config {
private:
	// static 成员类内初始化则赋的值必须为常量
	// 且对于非 const int 类型的 static 成员要类内初始化的化要加 constexpr 关键字
	constexpr static const char* db_name = "chat_geloutingyu";
	constexpr static const char* db_ip = "localhost";
	constexpr static const char* db_user = "root";
	constexpr static const char* db_passwd = "mysql";
	constexpr static const char* db_charset = "utf8";
	static const int db_port = 3306;

	// mysql 连接池的大小
	static const int max_size = 10;

public:
	config() {

	}

	~config() {

	}

	static const int get_max_size() {
		return max_size;
	}

	static const char* get_db_name() {
		return db_name;
	}

	static const char* get_db_ip() {
		return db_ip;
	}

	static const char* get_db_user() {
		return db_user;
	}

	static const char* get_db_passwd() {
		return db_passwd;
	}

	static const char* get_db_charset() {
		return db_charset;
	}

	static const int get_db_port() {
		return db_port;
	}
	
};


#endif