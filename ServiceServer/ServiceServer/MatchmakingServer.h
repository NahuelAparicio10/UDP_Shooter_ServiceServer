#pragma once
#include <SFML/Network.hpp>
#include <string>
#include <vector>
#include <queue>
#include <atomic>
#include <iostream>

constexpr int MAX_RETRIES = 5;
constexpr float RESEND_INTERVAL = 1.0f;

struct ClientMatchInfo 
{
	sf::IpAddress ip;
	unsigned short port;
};

struct PendingMatch
{
	ClientMatchInfo player;
	std::string matchMessage;
	int retries = 0;
	sf::Clock timer;
};

class MatchmakingServer
{
public:
	MatchmakingServer();
	~MatchmakingServer();

	void Run(std::atomic<bool>& running);

private:
	bool InitializeSocket();
	void HandleMessage(const std::string& message, const sf::IpAddress& sender, unsigned short port);
	void ProcessMatchmaking();
	void ProcessACKS();

	sf::UdpSocket _socket;
	unsigned short _port = 9100;

	std::queue<ClientMatchInfo> _normalQueue;
	std::queue<ClientMatchInfo> _rankedQueue;
	sf::Clock _matchmakingTimer;

	std::vector<PendingMatch> _pendingAcks;

};

