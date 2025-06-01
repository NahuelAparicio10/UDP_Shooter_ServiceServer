#include "LoginServer.h"

LoginServer::LoginServer()
{
    StartListening(50000);
}

LoginServer::~LoginServer()
{
}

void LoginServer::StartListening(unsigned short port)
{
    // Opens port to listen to connection

    if (_listener.listen(port) != sf::Socket::Status::Done)
    {
        WriteConsole("[LR_SERVER] Failed to bind port");
    }

    _selector.add(_listener);
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
        WriteConsole("[LR_SERVER] New client connected.");
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
            WriteConsole("[LR_SERVER] Failed to extract command");
            return;
        }

        if (command == "LOGIN" || command == "REGISTER")
        {
            std::string nick, pass;
            
            if (!(packet >> nick >> pass)) 
            {
                WriteConsole("[LR_SERVER] Failed to extract user & password");
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
        bool success = DatabaseManager::GetInstance().RegisterUser(nick, pass);
        response << (success ? "REGISTER_OK" : "REGISTER_FAIL");

        client->GetSocket()->send(response);
    }
    else if (command == "LOGIN")
    {
        bool success = DatabaseManager::GetInstance().LoginUser(nick, pass);
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
