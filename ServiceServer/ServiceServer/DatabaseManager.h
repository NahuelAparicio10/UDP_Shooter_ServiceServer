#pragma once
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include "bcrypt.h"

class DatabaseManager
{
public:
	DatabaseManager();
	~DatabaseManager();

	void ConnectDatabase();

	bool RegisterUser(const std::string& nickname, const std::string& password);
	bool LoginUser(const std::string& nickname, const std::string& password);

private:
	sql::Driver* _driver;
	sql::Connection* _con;
};

