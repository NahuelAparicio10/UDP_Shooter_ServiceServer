#pragma once
#include <SFML/Network.hpp>
#include <string>
#include <vector>
#include <queue>
#include <atomic>
#include <iostream>
#include "ConsoleUtils.h"
#include "PacketDispatcher.h"
#include "Constants.h"
#include "StartMatchData.h"

constexpr int MAX_RETRIES = 5;
constexpr float RESEND_INTERVAL = 1.0f;

class MatchmakingServer {
public:
    MatchmakingServer();
    ~MatchmakingServer();

    void Run(std::atomic<bool>& running);



private:
    void SubcribeActionHandlers();
    std::string GenerateMatchID();
    bool InitializeSocket();
    void ProcessMatchmaking(MatchQueue matchQueue);
    void ProcessMatchCreations();
    void ProcessMatchSessionsACKS();
    void RemoveSessionAndReQueue(const MatchSession& session);

    void CreateMatchSession(const StartMatchData& data);

    sf::UdpSocket _socket;

    std::queue<ClientMatchInfo> _normalQueue;
    std::queue<ClientMatchInfo> _rankedQueue;
    sf::Clock _matchmakingTimer;

    std::vector<MatchSession> _pendingSessions;
    unsigned int _playersPerMatch = 2;
    unsigned int _lastMatchID = 0;
    std::vector<PendingMatchCreation> _pendingMatchCreations;
    PacketDispatcher _dispatcher;
};
