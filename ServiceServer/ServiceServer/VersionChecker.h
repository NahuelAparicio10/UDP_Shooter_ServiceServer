#pragma once
#include <atomic>
#include <string>
#include <iostream>
#include <SFML/Network.hpp>
#include <fstream>
#include "ConsoleUtils.h"
#include "PacketDispatcher.h"
#include <sstream>
#include "Constants.h"
// -- Manages the versión client-map

class VersionChecker
{
public:
	VersionChecker();
	~VersionChecker();

	void Run(std::atomic<bool>& running);

private:
	std::string _lastestVersion;
	sf::UdpSocket _socket;

	bool InitializeSocket();
	std::string GetLocalVersion();
	//void HandleClient(const std::string& message, const sf::IpAddress& sender, unsigned short senderPort);
	void SendFile(sf::IpAddress address, unsigned short port);
	PacketDispatcher _dispatcher;
};



