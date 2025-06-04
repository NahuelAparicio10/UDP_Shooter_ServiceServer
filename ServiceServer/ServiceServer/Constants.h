#pragma once
#include <string>
#include "SFML/Network.hpp"

#pragma region Paths
inline static const std::string VersionFile = "Config/version.txt";
inline static const std::string MapFile = "Maps/map_v0_0.txt";
#pragma endregion

#pragma region IP / Ports

inline static const std::optional<sf::IpAddress> GameServerIP = sf::IpAddress::resolve("192.168.1.133");
inline static const unsigned short GameServerPort = 61000;
inline static const unsigned short VersionCheckerServerPort = 9000;
inline static const unsigned short MatchMakingServerPort = 9100;
#pragma endregion 