#pragma once
#include <atomic>

class LoginServer
{
public:
	LoginServer();
	~LoginServer();

	void Run(std::atomic<bool>& running);
};

