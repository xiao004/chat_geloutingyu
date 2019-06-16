#include <iostream>

#include "../mysql/user_table.h"

using namespace std;


void test_search_limit(const std::string column, const int page, const int cap) {

	mysqlpp::StoreQueryResult ares = user_table::search_limit(column, page, cap);
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


void test_search_where(const std::string column, const std::string where) {
	mysqlpp::StoreQueryResult ares = user_table::search_where(column, where);

	std::cout << ares[0][0] << " " << ares[0][1] << " " << ares[0][2] << std::endl;

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


void test_delete_where_uid_password(const int uid, const std::string password) {
	int cnt = user_table::delete_where_uid_password(uid, password);
	std::cout << cnt << std::endl;
}


void test_update_c_group_num_and_c_group(const int uid, const std::string &password, const int c_group_num, const std::string &c_group) {
	int cnt = user_table::update_c_group_num_and_c_group(uid, password, c_group_num, c_group);

	std::cout << cnt << std::endl;
}


int main(int argc, char const *argv[]) {
	
	// int flag =  user_table::insert("xiao004", "123456", "192.168.2.105:22345", "", "");

	// int flag =  user_table::insert("xiao006", "123456");

	// std::cout << flag << std::endl;

	// const std::string addr = "127.0.0.1:22334";

	// bool flag = user_table::update_online_and_addr(2, 1, "123456", "127.0.0.1:22334");

	// test_search_limit("uid,uname,online", 1, 20);

	// test_search_where("online", "uid=1");

	// test_delete_where_uid_password(10, "1234");

	// test_search_where("c_group_num,c_group_num_max,c_group", "uid=2");

	test_update_c_group_num_and_c_group(2, "123456", 1, "1");

	return 0;
}