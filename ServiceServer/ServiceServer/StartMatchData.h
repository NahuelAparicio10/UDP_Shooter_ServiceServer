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
    std::vector<ClientMatchInfo> players;
};

inline std::string SerializeMatch(const StartMatchData& data) {
    std::ostringstream ss;
    ss << data.matchID << ":" << (data.type == MatchType::RANKED ? "RANKED" : "NORMAL");
    for (const auto& p : data.players)
        ss << ":" << p.ip.toString() << ":" << p.port;
    return ss.str();
}

inline StartMatchData DeserializeMatch(const std::string& str) {
    StartMatchData data;
    std::istringstream ss(str);
    std::string token;
    std::getline(ss, token, ':');
    data.matchID = std::stoi(token);
    std::getline(ss, token, ':');
    data.type = token == "RANKED" ? MatchType::RANKED : MatchType::NORMAL;
    while (std::getline(ss, token, ':')) {
        std::optional<sf::IpAddress> ip = sf::IpAddress::resolve(token);
        std::getline(ss, token, ':');
        unsigned short port = std::stoi(token);
        data.players.push_back({ ip.value(), port});
    }
    return data;
}
