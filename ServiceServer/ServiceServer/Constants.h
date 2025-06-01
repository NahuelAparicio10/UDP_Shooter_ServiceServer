#pragma once
#include <string>
#include "SFML/Network.hpp"

#pragma region IP / Ports
inline static const std::optional<sf::IpAddress> GameServerIP = sf::IpAddress::resolve("127.0.0.1");
inline static const unsigned short GameServerPort = 50000;
inline static const unsigned short VersionCheckerServerPort = 9000;
inline static const unsigned short MatchMakingServerPort = 9100;
#pragma endregion 