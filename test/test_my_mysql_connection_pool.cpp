#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#include "../internal/my_mysql_connection_pool.h"

using namespace std;

std::shared_ptr<my_mysql_connection_pool> db_pool = nullptr;
std::string db_name = "chat_geloutingyu";
std::string db_ip = "localhost";
std::string db_user = "root";
std::string db_passwd = "mysql";
int db_port = 3306;
std::string db_charset = "utf8";
int max_size = 10;
int BUFF_SIZE = 1024;
char buff[1024];


std::string create_table_format = "create table %s;";
std::string check_table_format = "show tables like \"%s\";";
std::string insert_format = "insert into user(uname, password, addr, j_group, c_group, time) values(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d);";
std::string delete_format = "delete from user where uid = %d;";
std::string update_format = "update user set password = \"%s\" where uid = %d;";
std::string search_format = "select * from user;";


std::string get_check_string() {
	memset(buff, 0, BUFF_SIZE);
	std::string temp = "user";
	snprintf(buff, sizeof(buff), check_table_format.c_str(), temp);
	std::string buff_as_std_str = buff;
	return buff_as_std_str;
}


int check_table_exist(std::string sql) {
	mysqlpp::ScopedConnection conn(*db_pool, false);
	mysqlpp::Query query = conn->query();
	query << sql;

	if(query.exec()) {
		return 1;
	} else {
		return 0;
	}
}


std::string get_insert_string(std::string uname, std::string password, std::string addr, std::string j_gruop, std::string c_group) {
	memset(buff, 0, BUFF_SIZE);
	time_t t = (int)time(NULL);
	int tim = time(&t);

	snprintf(buff, sizeof(buff), insert_format.c_str(), uname.c_str(), password.c_str(), addr.c_str(), j_gruop.c_str(), c_group.c_str(), tim);

	std::string buff_as_std_str = buff;

	return buff_as_std_str;
}


bool insert(std::string sql) {
	mysqlpp::ScopedConnection conn(*db_pool, false);
	mysqlpp::Query query = conn->query();
	query << sql;

	mysqlpp::SimpleResult res = query.execute();

	if(res) {
		std::cout << "inserted into user table, insert_id = " << res.insert_id() << std::endl;
		return true;
	}
	return false;
}


std::string get_search_string() {
	return search_format;
}


void search(std::string sql) {
	mysqlpp::ScopedConnection conn(*db_pool, false);
	mysqlpp::Query query = conn->query();
	query << sql;

	mysqlpp::StoreQueryResult ares = query.store();
		if(ares) {
			std::cout << "ares.num_rows() = " << ares.num_rows() << std::endl;

			for(size_t i = 0; i < ares.num_rows(); ++i) {
				// std::cout << "uid: " << ares[i]["uid"] << "\tuname: " << ares[i]["uname"] << "\tpassword: " << ares[i]["password"] << "\tonline: " << ares[i]["online"] << std::endl;
				mysqlpp::Row row = ares[i];
				// cout << row[0] << " " << row[1] << endl;
				for(auto it = row.begin(); it != row.end(); ++it) {
					std::cout << *it << " ";
				}
				std::cout << std::endl;
			}
	}
}


std::string get_update_string(int uid, std::string password) {
	memset(buff, 0, BUFF_SIZE);

	snprintf(buff, sizeof(buff), update_format.c_str(), password.c_str(), uid);
	std::string buff_as_std_str = buff;

	return buff_as_std_str;
}


bool update(std::string sql) {
	mysqlpp::ScopedConnection conn(*db_pool, false);
	mysqlpp::Query query = conn->query();
	query << sql;

	if(query.exec()) {
		std::cout << "update success!" << std::endl;
		return true;
	}
	return false;
}


std::string get_delete_string(int uid) {
	memset(buff, 0, BUFF_SIZE);

	snprintf(buff, sizeof(buff), delete_format.c_str(), uid);
	std::string buff_as_std_str = buff;

	return buff_as_std_str;
}

bool delete_from(std::string sql) {
	mysqlpp::ScopedConnection conn(*db_pool, false);
	mysqlpp::Query query = conn->query();
	query << sql;

	if(query.exec()) {
		std::cout << "delete success!" << std::endl;
		return true;
	}
	return false;
}


int main(int argc, char const *argv[]) {

	db_pool = std::make_shared<my_mysql_connection_pool>(db_name, db_ip, db_user, db_passwd, db_port, db_charset, max_size);
	
	std::string insert_sql = get_insert_string("xiao004", "123456", "192.168.2.105:22345", "", "");
	std::cout << insert_sql << std::endl;
	// insert(insert_sql);
	std::cout << std::endl;

	std::string search_sql = get_search_string();
	std::cout << search_sql << std::endl;
	search(search_sql);
	std::cout << std::endl;

	std::string update_sql = get_update_string(1, "1234");
	std::cout << update_sql << std::endl;
	update(update_sql);
	std::cout << std::endl;

	search(search_sql);
	std::cout << std::endl;

	std::string delete_sql = get_delete_string(1);
	cout << delete_sql << endl;
	// delete_from(delete_sql);
	std::cout << std::endl;

	return 0;
}