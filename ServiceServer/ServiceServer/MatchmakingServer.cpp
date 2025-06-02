#include "MatchmakingServer.h"

MatchmakingServer::MatchmakingServer()
{
}

MatchmakingServer::~MatchmakingServer() { _socket.unbind(); }

void MatchmakingServer::SetPlayersPerMatch(unsigned int count) { _playersPerMatch = count; }

std::string MatchmakingServer::GenerateMatchID() {
    static int matchCounter = 0;
    return "MATCH_" + std::to_string(++matchCounter);
}

void MatchmakingServer::Run(std::atomic<bool>& running)
{
    if (!InitializeSocket()) return;

    // - Subscibe the content inside for when it's called by packet dispatcher (when received packet type FIND_MATCH)
    _dispatcher.RegisterHandler(PacketType::FIND_MATCH, [this](const RawPacketJob& job) {
        // - Check if the client wants to list in a casual game
        if (job.content == "NORMAL") 
        {
            _normalQueue.push({ job.sender.value(), job.port});
            WriteConsole("[MATCHMAKING] Player queued NORMAL: ", job.sender.value().toString(), ":", std::to_string(job.port), "  Queue size: ", _normalQueue.size());
            // - Send ACK 
            SendDatagram(_socket, PacketHeader::URGENT, PacketType::SEARCH_ACK, "", job.sender.value(), job.port);
        }
        // - Check if the client wants to list in a ranked game
        else if (job.content == "RANKED") 
        {
            _rankedQueue.push({ job.sender.value(), job.port});
            WriteConsole("[MATCHMAKING] Player queued NORMAL: ", job.sender.value().toString(), ":", std::to_string(job.port), "  Queue size: ", _normalQueue.size());
            // - Send ACK 
            SendDatagram(_socket, PacketHeader::URGENT, PacketType::SEARCH_ACK, "", job.sender.value(), job.port);
        }
    });

    // - Subscibe the content inside for when it's called by packet dispatcher (when received packet type CANCEL_MATCH)
        _dispatcher.RegisterHandler(PacketType::CANCEL_SEARCH, [this](const RawPacketJob& job) {
        // - OnCancel removes clients from queue and and send ACK (cancelled confirmation)
        auto removeFromQueue = [&](std::queue<ClientMatchInfo>& queue) {
            std::queue<ClientMatchInfo> temp;
            while (!queue.empty()) 
            {
                auto p = queue.front(); queue.pop();
                if (!(p.ip == job.sender.value() && p.port == job.port)) temp.push(p);
                else WriteConsole("[MATCHMAKING] Cancelled: ", job.sender.value(), ":", job.port);
            }
            queue = std::move(temp);
        };

        removeFromQueue(_normalQueue);
        removeFromQueue(_rankedQueue);

        char response[64];
        std::string payload = "";
        SendDatagram(_socket, PacketHeader::URGENT, PacketType::OK, payload, job.sender.value(), job.port);
        });

    // - Subscibe the content inside for when it's called by packet dispatcher (when received packet type ACK_FOUND)
        _dispatcher.RegisterHandler(PacketType::ACK_MATCH_FOUND, [this](const RawPacketJob& job) 
        {
                // - On ACK received remove session from resending match found
            for (auto& session : _pendingSessions) 
            {
                for (auto& p : session.players) {
                    if (p.player.ip == job.sender && p.player.port == job.port) 
                    {
                        p.ackRecieved = true;
                        WriteConsole("[MATCHMAKING] ACK received from ", job.sender.value(), ":", job.port);
                    }
                }
            }
        });

    _dispatcher.Start();
    // - Main loop where is receiveng the datagrams
    while (running)
    {
        char buffer[1024];
        std::size_t received;
        std::optional<sf::IpAddress> sender = std::nullopt;
        unsigned short port;
        _socket.setBlocking(false);
        if (_socket.receive(buffer, sizeof(buffer), received, sender, port) == sf::Socket::Status::Done)
        {
            RawPacketJob job;
            if (ParseRawDatagram(buffer, received, job, sender.value(), port))
                _dispatcher.EnqueuePacket(job);
        }

        // - Every 100ms checks if match is ready and processes pendingMatches ACK
        if (_matchmakingTimer.getElapsedTime().asMilliseconds() > 100)
        {
            ProcessMatchmaking({ MatchType::NORMAL, &_normalQueue });
            ProcessMatchmaking({ MatchType::RANKED, &_rankedQueue });
            ProcessMatchSessionsACKS();
            _matchmakingTimer.restart();
        }
    }
    _dispatcher.Stop();
}

// -- Binds UDP port for matchmaking 
bool MatchmakingServer::InitializeSocket()
{
	if (_socket.bind(MatchMakingServerPort) != sf::Socket::Status::Done)
	{
        WriteConsole("[MATCHMAKING_SERVER] Failed to bind UDP port ", MatchMakingServerPort);
		return false;
	}
    WriteConsole("[MATCHMAKING_SERVER] Listening on port ", MatchMakingServerPort);
	return true;
}


void MatchmakingServer::ProcessMatchmaking(MatchQueue matchQueue)
{    
    auto& queue = *matchQueue.queue;
    while (queue.size() >= _playersPerMatch)
    {
        // - Extract number of layers from queue and generates IDs
        std::vector<ClientMatchInfo> players;
        for (unsigned int i = 0; i < _playersPerMatch; ++i)
        {
            ClientMatchInfo info = queue.front(); queue.pop();
            info.playerID = i;
            players.push_back(info);
        }

        // - Tries to create an unique GameServerID
        std::string matchIDStr;
        unsigned int matchID;
        bool created = false;
        int attempts = 0;

        while (!created && attempts < 5)
        {
            matchIDStr = GenerateMatchID(); 
            matchID = std::stoi(matchIDStr.substr(6)); 

            StartMatchData matchData;
            matchData.matchID = matchID;
            matchData.type = matchQueue.type;
            matchData.players = players;
            matchData.numOfPlayers = _playersPerMatch;

            std::string serialized = SerializeMatch(matchData);
            SendDatagram(_socket, PacketHeader::URGENT, PacketType::CREATE_MATCH, serialized, GameServerIP.value(), GameServerPort);

            sf::Clock waitTimer;
            while (waitTimer.getElapsedTime().asSeconds() < 1.0f)
            {
                char buffer[1024];
                std::size_t received = 0;
                std::optional<sf::IpAddress> sender = std::nullopt;
                unsigned short port;
                _socket.setBlocking(false);

                // - While listening to GameServer port, if receives MATCH_UNIQUE go create match if not retries with other random MatchID
                if (_socket.receive(buffer, sizeof(buffer), received, sender, port) == sf::Socket::Status::Done &&
                    sender == GameServerIP && port == GameServerPort)
                {
                    RawPacketJob job;
                    if (ParseRawDatagram(buffer, received, job, sender.value(), port))
                    {
                        if (job.type == PacketType::MATCH_UNIQUE)
                        {
                            created = true;
                            break;
                        }
                        else if (job.type == PacketType::MATCH_USED)
                        {
                            WriteConsole("[MATCHMAKING_SERVER] Match ID already in use, retrying...");
                            break;
                        }
                    }
                }

                sf::sleep(sf::milliseconds(100));
            }

            ++attempts;
        }

        if (!created)
        {
            WriteConsole("[MATCHMAKING_SERVER] Failed to create unique match after retries.");
            for (auto& p : players) queue.push(p);
            return;
        }

        // - Notifies to clients with IP:PORT:MATCHID:PLAYERID
        MatchSession session;
        for (const auto& p : players)
        {
            std::string msg = GameServerIP.value().toString() + ":" +
                std::to_string(GameServerPort) + ":" +
                std::to_string(matchID) + ":" +
                std::to_string(p.playerID);

            session.players.push_back({ p, msg, 0, sf::Clock(), false, matchQueue.type });

            SendDatagram(_socket, PacketHeader::URGENT, PacketType::MATCH_FOUND, msg, p.ip, p.port);
            WriteConsole("[MATCHMAKING_SERVER] Player id ", p.playerID , "' with match id ", matchID, "\n");
        }

        _pendingSessions.push_back(session);
        WriteConsole("[MATCHMAKING_SERVER] Created match '", matchID, "' with ", players.size(), " players.");
    }
}

// -- Procceses resending the matchfound if client / ack is not being found
void MatchmakingServer::ProcessMatchSessionsACKS()
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
                        // - If player doesn't send ACK confirmation in a specific period time, remove player form queue (RemoveSessionAndReQueue)
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
            // Aqu� puedes iniciar la l�gica real de juego o notificar al game server
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
