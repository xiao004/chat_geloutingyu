#include <iostream>

#include "../mysql/group_table.h"

using namespace std;


void test_insert(const int uid, const std::string &gname) {
	int id = group_table::insert(uid, gname);

	std::cout << id << std::endl;
}


int main(int argc, char const *argv[]) {

	test_insert(2, "family");

	return 0;
}