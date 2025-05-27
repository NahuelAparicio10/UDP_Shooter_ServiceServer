#pragma once
#include <SFML/Network.hpp>
#include <string>
#include <vector>
#include <queue>
#include <atomic>
#include <iostream>

constexpr int MAX_RETRIES = 5;
constexpr float RESEND_INTERVAL = 1.0f;

enum class MatchType { NORMAL, RANKED };

struct ClientMatchInfo {
    sf::IpAddress ip;
    unsigned short port;
};

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
    bool InitializeSocket();
    void HandleMessage(const std::string& message, const sf::IpAddress& sender, unsigned short port);
    void ProcessMatchmaking(MatchQueue matchQueue);
    void ProcessACKS();
    void RemoveSessionAndReQueue(const MatchSession& session);

    sf::UdpSocket _socket;
    unsigned short _port = 9100;

    std::queue<ClientMatchInfo> _normalQueue;
    std::queue<ClientMatchInfo> _rankedQueue;
    sf::Clock _matchmakingTimer;

    std::vector<MatchSession> _pendingSessions;

    unsigned int _playersPerMatch = 3;
};
