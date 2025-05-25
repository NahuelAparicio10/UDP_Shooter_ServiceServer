#include "LoginServer.h"

LoginServer::LoginServer()
{
    _database = new DatabaseManager();
    _database->ConnectDatabase();
}

LoginServer::~LoginServer()
{
    delete _database;
}

// -- Accepts new clients and adds them to the selector

void LoginServer::AcceptNewConnection()
{
    sf::TcpSocket* socket = new sf::TcpSocket();

    if (_listener.accept(*socket) == sf::Socket::Status::Done)
    {
        socket->setBlocking(false);
        _selector.add(*socket);
        _clients.emplace_back(std::make_unique<ClientLR>(socket));
        std::cout << "[Server] New client connected." << std::endl;
    }
    else 
    {
        delete socket;
    }
}

// -- Receives data from a client and processes the command

void LoginServer::ReceiveData(ClientLR* client)
{
    sf::Packet packet;

    if (client->GetSocket()->receive(packet) == sf::Socket::Status::Done)
    {
        std::string command;

        if (!(packet >> command))
        {
            std::cerr << "[LR_SERVER] Failed to extract command" << std::endl;
            return;
        }

        if (command == "LOGIN" || command == "REGISTER")
        {
            std::string nick, pass;
            
            if (!(packet >> nick >> pass)) 
            {
                std::cerr << "[LR_SERVER] Failed to extract user & password" << std::endl;
                return;
            }

            HandleCommand(client, command, nick, pass);
        }
    }
}

// -- Handles Login and Register checking DDBBManager

void LoginServer::HandleCommand(ClientLR* client, const std::string& command, const std::string& nick, const std::string& pass)
{
    sf::Packet response;

    if (command == "REGISTER")
    {
        bool success = _database->RegisterUser(nick, pass);
        response << (success ? "REGISTER_OK" : "REGISTER_FAIL");

        client->GetSocket()->send(response);
    }
    else if (command == "LOGIN")
    {
        bool success = _database->LoginUser(nick, pass);
        response << (success ? "LOGIN_OK" : "LOGIN_FAIL");

        client->GetSocket()->send(response);
    }
}

// -- Handles Update Thread for LoginServer checking new connections and incoming data

void LoginServer::Run(std::atomic<bool>& running)
{
	while (running)
	{
        if (_selector.wait(sf::seconds(1.f)))
        {
            if (_selector.isReady(_listener))
            {
                AcceptNewConnection();
            }
            else
            {
                for (auto& client : _clients)
                {
                    if (_selector.isReady(*client->GetSocket()))
                    {
                        ReceiveData(client.get());
                    }
                }
            }
        }
	}
}
