#include "ClientLR.h"

ClientLR::ClientLR(sf::TcpSocket* socket) : _socket(socket) 
{
    std::ostringstream ss;
    ss << "C" << std::rand();
    _guid = ss.str();
}

sf::TcpSocket* ClientLR::GetSocket() const { return _socket; }

std::string ClientLR::GetGUID() const { return _guid; }

std::string ClientLR::GetNickname() const { return _nickname; }

void ClientLR::SetNickname(const std::string& nickname) { _nickname = nickname; }

void ClientLR::SetRoomID(const std::string& roomID) { _roomID = roomID; }

std::string ClientLR::GetRoomID() const { return _roomID; }