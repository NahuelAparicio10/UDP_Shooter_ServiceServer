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
	const std::string _lastestVersion = "0.43";
	const std::string _mapFilePath = "Maps/map_v0_0.txt";
	sf::UdpSocket _socket;

	bool InitializeSocket();
	//void HandleClient(const std::string& message, const sf::IpAddress& sender, unsigned short senderPort);
	void SendFile(sf::IpAddress address, unsigned short port);
	PacketDispatcher _dispatcher;
};



