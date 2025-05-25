#pragma once
#include <atomic>
#include <vector>
#include "DatabaseManager.h"
#include "ClientLR.h"
#include <SFML/Network.hpp>

class LoginServer
{
public:
	LoginServer();
	~LoginServer();

	void AcceptNewConnection();
	void ReceiveData(ClientLR* client);
	void HandleCommand(ClientLR* client, const std::string& command, const std::string& nick, const std::string& pass);

	void Run(std::atomic<bool>& running);

private:
	sf::TcpListener _listener;
	sf::SocketSelector _selector;
	DatabaseManager* _database;
	std::vector<std::unique_ptr<ClientLR>> _clients;
};

