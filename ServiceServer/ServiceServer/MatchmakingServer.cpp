#include "MatchmakingServer.h"

MatchmakingServer::MatchmakingServer() { }

MatchmakingServer::~MatchmakingServer() { _socket.unbind(); }

// -- Binds UDP port for matchmaking 
bool MatchmakingServer::InitializeSocket()
{
    if (_socket.bind(MatchMakingServerPort) != sf::Socket::Status::Done)
    {
        WriteConsole("[MATCHMAKING_SERVER] Failed to bind UDP port ", MatchMakingServerPort);
        return false;
    }
    WriteConsole("[MATCHMAKING_SERVER] Listening on port ", MatchMakingServerPort);
    _socket.setBlocking(false);
    return true;
}

// -- Threads Main Loop

void MatchmakingServer::Run(std::atomic<bool>& running)
{
    if (!InitializeSocket()) return;

    SubcribeActionHandlers();

    _dispatcher.Start();

    // - Main loop where is receiveng the datagrams
    while (running)
    {
        char buffer[1024];
        std::size_t received;
        std::optional<sf::IpAddress> sender = std::nullopt;
        unsigned short port;

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
            ProcessMatchCreations(); 
            ProcessMatchSessionsACKS();
            _matchmakingTimer.restart();
        }
    }

    _dispatcher.Stop();
}

void MatchmakingServer::SubcribeActionHandlers()
{
    // - Subscibe the content inside for when it's called by packet dispatcher (when received packet type FIND_MATCH)
    _dispatcher.RegisterHandler(PacketType::FIND_MATCH, [this](const RawPacketJob& job) 
        {
            // - Check if the client wants to list in a casual game
            if (job.content == "NORMAL")
            {
                _normalQueue.push({ job.sender.value(), job.port });
                WriteConsole("[MATCHMAKING] Player queued NORMAL: ", job.sender.value().toString(), ":", std::to_string(job.port), "  Queue size: ", _normalQueue.size());

                SendDatagram(_socket, PacketHeader::URGENT, PacketType::SEARCH_ACK, "", job.sender.value(), job.port);
            }
            // - Check if the client wants to list in a ranked game
            else if (job.content == "RANKED")
            {
                _rankedQueue.push({ job.sender.value(), job.port });
                WriteConsole("[MATCHMAKING] Player queued NORMAL: ", job.sender.value().toString(), ":", std::to_string(job.port), "  Queue size: ", _normalQueue.size());

                SendDatagram(_socket, PacketHeader::URGENT, PacketType::SEARCH_ACK, "", job.sender.value(), job.port);
            }
        });

    // - Subscibe the content inside for when it's called by packet dispatcher (when received packet type CANCEL_MATCH)
    _dispatcher.RegisterHandler(PacketType::CANCEL_SEARCH, [this](const RawPacketJob& job) 
        {
            // - OnCancel removes clients from queue and and send ACK (cancelled confirmation)
            auto removeFromQueue = [&](std::queue<ClientMatchInfo>& queue) 
                {
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

    // - Subscibe the content inside for when it's called by packet dispatcher (when received packet type MATCH_UNIQUE)
    _dispatcher.RegisterHandler(PacketType::MATCH_UNIQUE, [this](const RawPacketJob& job) 
        {
            // - Confirms match creation
            for (auto it = _pendingMatchCreations.begin(); it != _pendingMatchCreations.end(); ++it)
            {
                StartMatchData& data = it->matchData;
                if (data.matchID == std::stoi(job.content)) 
                {
                    CreateMatchSession(data);
                    _pendingMatchCreations.erase(it);
                    break;
                }
            }
        });

    // - Subscibe the content inside for when it's called by packet dispatcher (when received packet type MATCH_USED)
    _dispatcher.RegisterHandler(PacketType::MATCH_USED, [this](const RawPacketJob& job) 
        {
            for (auto& creation : _pendingMatchCreations)
            {
                if (creation.matchData.matchID == std::stoi(job.content))
                {
                    // - Retries by generating a new matchID
                    creation.matchData.matchID = ++_lastMatchID;
                    creation.serialized = SerializeMatch(creation.matchData);
                    creation.attempts = 0;
                    creation.timer.restart();
                    break;
                }
            }
        });
}

// -- Generates a match ID type : "MATCH_" + incrementalnumber
std::string MatchmakingServer::GenerateMatchID()
{
    static int matchCounter = 0;
    return "MATCH_" + std::to_string(++matchCounter);
}

// -- Processes de matchmaking checking if there is enough players in queue to start a match

void MatchmakingServer::ProcessMatchmaking(MatchQueue matchQueue)
{
    auto& queue = *matchQueue.queue;

    while (queue.size() >= _playersPerMatch)
    {
        std::vector<ClientMatchInfo> players;

        for (unsigned int i = 0; i < _playersPerMatch; ++i)
        {
            ClientMatchInfo info = queue.front(); queue.pop();
            info.playerID = i;
            players.push_back(info);
        }

        StartMatchData matchData;
        matchData.matchID = ++_lastMatchID;
        matchData.type = matchQueue.type;
        matchData.numOfPlayers = _playersPerMatch;
        matchData.players = players;

        std::string serialized = SerializeMatch(matchData);

        _pendingMatchCreations.push_back(PendingMatchCreation{ matchData, serialized, 0, sf::Clock(), true });
    }
}

// -- Tries to create match with unique id, if not retries for 5 times and if it fails resends players to the queue

void MatchmakingServer::ProcessMatchCreations()
{
    for (auto it = _pendingMatchCreations.begin(); it != _pendingMatchCreations.end();)
    {
        auto& creation = *it;

        if (creation.timer.getElapsedTime().asMilliseconds() < 500) 
        {
            ++it;
            continue;
        }

        if (creation.attempts >= 5) 
        {
            WriteConsole("[MATCHMAKING_SERVER] Failed to create unique match after retries.");
            for (auto& p : creation.matchData.players)
            {
                if (creation.matchData.type == MatchType::NORMAL)
                    _normalQueue.push(p);
                else
                    _rankedQueue.push(p);
            }
            it = _pendingMatchCreations.erase(it);
            continue;
        }

        SendDatagram(_socket, PacketHeader::URGENT, PacketType::CREATE_MATCH,
            creation.serialized, GameServerIP.value(), GameServerPort);

        creation.timer.restart();
        
        creation.attempts++;

        ++it;
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

void MatchmakingServer::CreateMatchSession(const StartMatchData& data)
{
    MatchSession session;
    for (const auto& p : data.players)
    {
        std::string msg = GameServerIP.value().toString() + ":" +
            std::to_string(GameServerPort) + ":" +
            std::to_string(data.matchID) + ":" +
            std::to_string(p.playerID);

        session.players.push_back({ p, msg, 0, sf::Clock(), false, data.type });

        SendDatagram(_socket, PacketHeader::URGENT, PacketType::MATCH_FOUND, msg, p.ip, p.port);
    }
    _pendingSessions.push_back(session);
    WriteConsole("[MATCHMAKING_SERVER] Created match '", data.matchID, "' with ", data.players.size(), " players.");
}
