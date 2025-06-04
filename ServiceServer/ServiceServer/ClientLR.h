#pragma once
#include <SFML/Network.hpp>
#include <string>
#include <sstream>

// -- Save Client Data for Login & Register connections and communications

class ClientLR
{
public:
    ClientLR(sf::TcpSocket* socket);

    sf::TcpSocket* GetSocket() const;

    std::string GetGUID() const;
    std::string GetNickname() const;
    void SetNickname(const std::string& nickname);
    void SetRoomID(const std::string& roomID);
    std::string GetRoomID() const;

private:
    sf::TcpSocket* _socket;
    std::string _guid;
    std::string _nickname;
    std::string _roomID;
};

