#pragma once
#include <atomic>
class MatchmakingServer
{
public:
	MatchmakingServer();
	~MatchmakingServer();

	void Run(std::atomic<bool>& running);
};

