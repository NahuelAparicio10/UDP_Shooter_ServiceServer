#pragma once
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include "bcrypt.h"
#include <mutex>

class DatabaseManager
{
public:
	static DatabaseManager& GetInstance();

	void ConnectDatabase();

	bool RegisterUser(const std::string& nickname, const std::string& password);
	bool LoginUser(const std::string& nickname, const std::string& password);

	// - ELO
	int GetElo(const std::string& user);
	bool UpdateElo(const std::string& user, int newElo);

private:
	DatabaseManager();
	~DatabaseManager();

	// Deletes a copy of DDBB
	DatabaseManager(const DatabaseManager&) = delete;
	DatabaseManager& operator=(const DatabaseManager&) = delete;

	sql::Driver* _driver;
	sql::Connection* _con;

	std::mutex _loginMutex;
	std::mutex _eloMutex;
};

