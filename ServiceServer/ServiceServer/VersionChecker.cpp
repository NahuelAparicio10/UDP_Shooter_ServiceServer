#include "VersionChecker.h"

VersionChecker::VersionChecker() { }

VersionChecker::~VersionChecker() { _socket.unbind(); }

bool VersionChecker::InitializeSocket()
{
	if (_socket.bind(_port) != sf::Socket::Status::Done) 
	{
		std::cerr << "[VersionChecker] Failed to bind to port " << _port << std::endl;
		return false;
	}
	std::cerr << "[VersionChecker] Listening on port " << _port << std::endl;
	
	return true;
}

void VersionChecker::Run(std::atomic<bool>& running)
{
	if (!InitializeSocket()) return;

	char data[1024];
	std::size_t received;
	std::optional<sf::IpAddress> sender = std::nullopt;
	unsigned short senderPort;

	while (running) 
	{
		sf::Socket::Status status = _socket.receive(data, sizeof(data), received, sender, senderPort);
		
		if (status == sf::Socket::Status::Done && sender.has_value()) 
		{
			std::string message(data, received);
			HandleClient(message, sender.value(), senderPort);
		}
	}
}
void VersionChecker::HandleClient(const std::string& message, const sf::IpAddress& sender, unsigned short senderPort)
{
	std::cout << "[VERSION_CHECKER] Received from " << sender << ":" << senderPort << " -> " << message << std::endl;
	
	if (message.rfind("VERSION:", 0) == 0) 
	{
		std::string version = message.substr(8);

		if (version == _lastestVersion) 
		{
			std::string ok = "OK";
			_socket.send(ok.c_str(), ok.size(), sender, senderPort);
		}
		else 
		{
			std::string update = "UPDATE:" + _lastestVersion;
			_socket.send(update.c_str(), update.size(), sender, senderPort);

			SendFile(sender, senderPort);
		}
	}
}

void VersionChecker::SendFile(sf::IpAddress address, unsigned short port)
{
	std::ifstream file(_mapFilePath);

	if (!file.is_open()) 
	{
		std::cerr << "[VERSION_CHECKER] Could not open map file: " << _mapFilePath << std::endl;
		return;
	}

	std::string line;
	while (std::getline(file, line)) 
	{
		_socket.send(line.c_str(), line.size(), address, port);

		sf::sleep(sf::milliseconds(10)); // Evita saturar la red
	}

	std::string end = "EOF";
	_socket.send(end.c_str(), end.size(), address, port);

	std::cout << "[VERSION_CHECKER] Map sent to " << address << ":" << port << std::endl;
}

