#include "MatchmakingServer.h"

MatchmakingServer::MatchmakingServer()
{
}

MatchmakingServer::~MatchmakingServer()
{
	_socket.unbind();
}

void MatchmakingServer::Run(std::atomic<bool>& running)
{
    if (!InitializeSocket()) return;

    char buffer[1024];
    std::size_t received;
    std::optional<sf::IpAddress> sender = std::nullopt;
    unsigned short senderPort;

    while (running)
    {
        // Revisa mensajes entrantes
        if (_socket.receive(buffer, sizeof(buffer), received, sender, senderPort) == sf::Socket::Status::Done) {
            std::string message(buffer, received);
            HandleMessage(message, sender.value(), senderPort);
        }

        // Lógica de matchmaking cada 100 ms
        if (_matchmakingTimer.getElapsedTime().asMilliseconds() > 100) {
            ProcessMatchmaking();
            _matchmakingTimer.restart();
        }
    }
}

// -- Binds UDP port for matchmaking 

bool MatchmakingServer::InitializeSocket()
{
	if (_socket.bind(_port) != sf::Socket::Status::Done)
	{
		std::cerr << "[MATCHMAKING_SERVER] Failed to bind UDP port " << _port << std::endl;
		return false;
	}
	std::cout << "[MATCHMAKING_SERVER] Listening on port " << _port << std::endl;
	return true;
}

// -- Handles UDP mesasge recived and checks if its a NORMAL or RANKED find match or any

void MatchmakingServer::HandleMessage(const std::string& message, const sf::IpAddress& sender, unsigned short port)
{
    if (message == "FIND_MATCH:NORMAL") 
    {
        std::cout << "[MATCHMAKING_SERVER] Player queued: " << sender.toString() << ":" << port << std::endl;
        _normalQueue.push({ sender, port });
    }
    else if (message == "FIND_MATCH:RANKED")
    {
        std::cout << "[MATCHMAKING_SERVER] Player queued: " << sender.toString() << ":" << port << std::endl;
        _rankedQueue.push({ sender, port });
    }
    else if (message == "ACK_MATCH_FOUND")
    {
        auto it = std::find_if(_pendingAcks.begin(), _pendingAcks.end(),
            [&](const PendingMatch& p) {
                return p.player.ip == sender && p.player.port == port;
            });

        if (it != _pendingAcks.end())
        {
            std::cout << "[MATCHMAKING_SERVER] ACK received from " << sender.toString() << ":" << port << "\n";
            _pendingAcks.erase(it);
        }
    }
}

void MatchmakingServer::ProcessMatchmaking()
{
    while (_normalQueue.size() >= 2)
    {
        ClientMatchInfo player1 = _normalQueue.front(); _normalQueue.pop();
        ClientMatchInfo player2 = _normalQueue.front(); _normalQueue.pop();

        std::string matchInfo = "MATCH_FOUND:127.0.0.1:60000"; 

        _pendingAcks.push_back({ player1, matchInfo, 0, sf::Clock() });
        _pendingAcks.push_back({ player2, matchInfo, 0, sf::Clock() });

        std::cout << "[MATCHMAKING_SERVER] Match found. Waiting for ACKs from:\n";

        std::cout << "  - " << player1.ip << ":" << player1.port << "\n";
        std::cout << "  - " << player2.ip << ":" << player2.port << "\n";
    }
}

// -- Procceses resending the matchfound if client / ack is not being found

void MatchmakingServer::ProcessACKS()
{
    for (auto it = _pendingAcks.begin(); it != _pendingAcks.end(); )
    {
        if (it->timer.getElapsedTime().asSeconds() > RESEND_INTERVAL)
        {
            if (it->retries >= MAX_RETRIES)
            {
                std::cout << "[MATCHMAKING_SERVER] ACK not received after retries. Giving up on " << it->player.ip << ":" << it->player.port << std::endl;
                it = _pendingAcks.erase(it);
                continue;
            }
            _socket.send(it->matchMessage.c_str(), it->matchMessage.size(), it->player.ip, it->player.port);

            it->timer.restart();
            it->retries++;
        }
        ++it;
    }
}
