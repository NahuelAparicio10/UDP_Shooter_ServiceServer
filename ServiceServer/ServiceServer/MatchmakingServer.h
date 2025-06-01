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



struct PendingMatch {
    ClientMatchInfo player;
    std::string matchMessage;
    int retries = 0;
    sf::Clock timer;
    bool ackRecieved = false;
    MatchType matchType;
};

struct MatchSession {
    std::vector<PendingMatch> players;
};

struct MatchQueue {
    MatchType type;
    std::queue<ClientMatchInfo>* queue;
};

class MatchmakingServer {
public:
    MatchmakingServer();
    ~MatchmakingServer();

    void Run(std::atomic<bool>& running);
    void SetPlayersPerMatch(unsigned int count);


private:
    std::string GenerateMatchID();
    bool InitializeSocket();
    void ProcessMatchmaking(MatchQueue matchQueue);
    void ProcessACKS();
    void RemoveSessionAndReQueue(const MatchSession& session);

    sf::UdpSocket _socket;

    std::queue<ClientMatchInfo> _normalQueue;
    std::queue<ClientMatchInfo> _rankedQueue;
    sf::Clock _matchmakingTimer;

    std::vector<MatchSession> _pendingSessions;
    unsigned int _playersPerMatch = 2;

    PacketDispatcher _dispatcher;
};
