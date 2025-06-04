#include "ServiceManager.h"

ServiceManager::ServiceManager()
{
	DatabaseManager::GetInstance().ConnectDatabase();

	_versionChecker = new VersionChecker();
	_loginServer = new LoginServer();
	_matchMakingServer = new MatchmakingServer();
	_running = true;
}

ServiceManager::~ServiceManager()
{
	StopServices();
	delete _versionChecker;
	delete _loginServer;
	delete _matchMakingServer;
}

// -- Creates one thread for each service of the server (Version Checker, Login & Register and Match Making)

void ServiceManager::InitializeServices()
{
	_versionThread = std::thread(&VersionChecker::Run, _versionChecker, std::ref(_running));
	_loginRegisterThread = std::thread(&LoginServer::Run, _loginServer, std::ref(_running));
	_matchmakingThread = std::thread(&MatchmakingServer::Run, _matchMakingServer, std::ref(_running));
}

//-- Stops all the services

void ServiceManager::StopServices()
{
	_running = false;

	if (_versionThread.joinable()) _versionThread.join();
	if (_loginRegisterThread.joinable()) _loginRegisterThread.join();
	if (_matchmakingThread.joinable()) _matchmakingThread.join();
}


