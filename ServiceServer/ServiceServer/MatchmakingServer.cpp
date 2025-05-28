#include "MatchmakingServer.h"

MatchmakingServer::MatchmakingServer()
{
}

MatchmakingServer::~MatchmakingServer() { _socket.unbind(); }

void MatchmakingServer::SetPlayersPerMatch(unsigned int count) { _playersPerMatch = count; }

void MatchmakingServer::Run(std::atomic<bool>& running)
{
    if (!InitializeSocket()) return;

    char buffer[1024];
    std::size_t received;
    std::optional<sf::IpAddress> sender = std::nullopt;
    unsigned short senderPort;

    while (running)
    {
        if (_socket.receive(buffer, sizeof(buffer), received, sender, senderPort) == sf::Socket::Status::Done) {
            std::string message(buffer, received);
            HandleMessage(message, sender.value(), senderPort);
        }

        if (_matchmakingTimer.getElapsedTime().asMilliseconds() > 100)
        {
            ProcessMatchmaking({ MatchType::NORMAL, &_normalQueue });
            ProcessMatchmaking({ MatchType::RANKED, &_rankedQueue });
            ProcessACKS();
            _matchmakingTimer.restart();
        }
    }
}

// -- Binds UDP port for matchmaking 

bool MatchmakingServer::InitializeSocket()
{
	if (_socket.bind(_port) != sf::Socket::Status::Done)
	{
        WriteConsole("[MATCHMAKING_SERVER] Failed to bind UDP port ", _port);
		return false;
	}
    WriteConsole("[MATCHMAKING_SERVER] Listening on port ", _port);
	return true;
}

// -- Handles UDP mesasge recived and checks if its a NORMAL or RANKED find match or any

void MatchmakingServer::HandleMessage(const std::string& message, const sf::IpAddress& sender, unsigned short port)
{
    if (message == "FIND_MATCH:NORMAL")
    {
        _normalQueue.push({ sender, port });
        WriteConsole("[MATCHMAKING_SERVER] Player queued NORMAL: ", sender, ":", port);
    }
    else if (message == "FIND_MATCH:RANKED")
    {
        _rankedQueue.push({ sender, port });
        WriteConsole("[MATCHMAKING_SERVER] Player queued RANKED: ", sender, ":", port);
    }
    else if (message == "CANCELED_SEARCHING")
    {
        auto removeFromQueue = [&](std::queue<ClientMatchInfo>& queue) {
            std::queue<ClientMatchInfo> tempQueue;
            while (!queue.empty()) {
                ClientMatchInfo current = queue.front(); queue.pop();
                if (!(current.ip == sender && current.port == port)) 
                {
                    tempQueue.push(current);
                }
                else 
                {
                    WriteConsole("[MATCHMAKING_SERVER] Player canceled search: ", sender, ":", port);
                }
            }
            queue = std::move(tempQueue);
        };

        removeFromQueue(_normalQueue);
        removeFromQueue(_rankedQueue);

        std::string confirm = "CANCEL_CONFIRMED";
        _socket.send(confirm.c_str(), confirm.size(), sender, port);
    }
    else if (message == "ACK_MATCH_FOUND")
    {
        for (auto& session : _pendingSessions)
        {
            for (auto& p : session.players)
            {
                if (p.player.ip == sender && p.player.port == port)
                {
                    p.ackRecieved = true;
                    WriteConsole("[MATCHMAKING_SERVER] ACK received from ", sender, ":", port);
                }
            }
        }
    }
}

void MatchmakingServer::ProcessMatchmaking(MatchQueue matchQueue)
{
    auto& queue = *matchQueue.queue;
    while (queue.size() >= _playersPerMatch)
    {
        MatchSession session;
        std::string matchMessage = std::string("MATCH_FOUND:127.0.0.1:60000:") + (matchQueue.type == MatchType::RANKED ? "RANKED" : "NORMAL");
        
        for (unsigned int i = 0; i < _playersPerMatch; ++i)
        {
            ClientMatchInfo player = queue.front(); queue.pop();
            session.players.push_back({ player, matchMessage, 0, sf::Clock(), false, matchQueue.type });
            _socket.send(matchMessage.c_str(), matchMessage.size(), player.ip, player.port);
        }

        _pendingSessions.push_back(session);

        WriteConsole("[MATCHMAKING_SERVER] ", _playersPerMatch, " Player ", 
            (matchQueue.type == MatchType::RANKED ? "[RANKED]" : "[NORMAL]"), " Match created.");
    }
}

// -- Procceses resending the matchfound if client / ack is not being found

void MatchmakingServer::ProcessACKS()
{
    for (auto it = _pendingSessions.begin(); it != _pendingSessions.end(); )
    {
        bool allAcked = true;
        bool giveUp = false;

        for (auto& p : it->players)
        {
            if (!p.ackRecieved)
            {
                allAcked = false;

                if (p.timer.getElapsedTime().asSeconds() > RESEND_INTERVAL)
                {
                    if (p.retries >= MAX_RETRIES)
                    {
                        WriteConsole("[MATCHMAKING_SERVER] Giving up on player ", p.player.ip, ":", p.player.port);
                        giveUp = true;
                        break;
                    }

                    _socket.send(p.matchMessage.c_str(), p.matchMessage.size(), p.player.ip, p.player.port);
                    p.timer.restart();
                    p.retries++;
                }
            }
        }

        if (giveUp)
        {
            RemoveSessionAndReQueue(*it);
            it = _pendingSessions.erase(it);
        }
        else if (allAcked)
        {
            WriteConsole("[MATCHMAKING_SERVER] All ACKs received. Starting match...");
            // Aquí puedes iniciar la lógica real de juego o notificar al game server
            it = _pendingSessions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// -- If any player in your match disconnects / does not reply ACK it earease that player an requeue the other ones

void MatchmakingServer::RemoveSessionAndReQueue(const MatchSession& session)
{
    for (const auto& p : session.players)
    {
        if (p.ackRecieved)
        {
            WriteConsole("[MATCHMAKING_SERVER] Returning ", p.player.ip, ":", p.player.port, " to queue.");
            if (p.matchType == MatchType::NORMAL)
                _normalQueue.push(p.player);
            else
                _rankedQueue.push(p.player);
        }
    }
}
