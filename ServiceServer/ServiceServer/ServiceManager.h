#pragma once
#include <thread>
#include <atomic>
#include "VersionChecker.h"
#include "MatchmakingServer.h"
#include "LoginServer.h"

// -- Initialize everything and launches the threads

class ServiceManager
{
public:
	ServiceManager();
	~ServiceManager();

	void InitializeServices();
	void StopServices();

private:
	std::thread _versionThread;
	std::thread _loginRegisterThread;
	std::thread _matchmakingThread;

	std::atomic<bool> _running;

	VersionChecker* _versionChecker;
	LoginServer* _loginServer;
	MatchmakingServer* _matchMakingServer;

};

