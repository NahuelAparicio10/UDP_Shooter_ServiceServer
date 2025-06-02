#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <string>
#include <sstream>

enum class MatchType { NORMAL, RANKED };

struct ClientMatchInfo {
    sf::IpAddress ip;
    unsigned short port;
    unsigned int playerID = 0;
};

struct StartMatchData {
    unsigned int matchID;
    MatchType type;
    int numOfPlayers;
    std::vector<ClientMatchInfo> players;
};
struct PendingMatchCreation {
    StartMatchData matchData;
    std::string serialized = "";
    int attempts = 0;
    sf::Clock timer;
    bool waitingForAck = true;
};



// -- Saves the match data into string to be sent
inline std::string SerializeMatch(const StartMatchData& data) {
    std::ostringstream ss;
    ss << data.matchID << ":" << (data.type == MatchType::RANKED ? "RANKED" : "NORMAL") << ":" << data.numOfPlayers;
    for (const auto& p : data.players)
        ss << ":" << p.ip.toString() << ":" << p.port << ":" << p.playerID;
    return ss.str();
}


