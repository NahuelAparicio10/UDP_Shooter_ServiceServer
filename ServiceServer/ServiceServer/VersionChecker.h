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

// -- Manages the version control and sends map to the clients if they are not updated

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
	void SendFile(sf::IpAddress address, unsigned short port);
	PacketDispatcher _dispatcher;
};



