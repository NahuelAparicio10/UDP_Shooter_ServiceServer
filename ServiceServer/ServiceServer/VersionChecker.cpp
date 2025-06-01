#include "VersionChecker.h"

VersionChecker::VersionChecker() { }

VersionChecker::~VersionChecker() { _socket.unbind(); }

bool VersionChecker::InitializeSocket()
{
	if (_socket.bind(VersionCheckerServerPort) != sf::Socket::Status::Done)
	{
		WriteConsole("[VersionChecker] Failed to bind to port ", VersionCheckerServerPort);
		return false;
	}

	WriteConsole("[VersionChecker] Listening on port ", VersionCheckerServerPort);
	
	return true;
}

void VersionChecker::Run(std::atomic<bool>& running)
{
	if (!InitializeSocket()) return;

	//char data[1024];
	//std::size_t received;
	//std::optional<sf::IpAddress> sender = std::nullopt;
	//unsigned short senderPort;

	_dispatcher.RegisterHandler(PacketType::VERSION, [this](const RawPacketJob& job) {
		std::string version = job.content;

		if (version == _lastestVersion) 
		{
			char buffer[64];
			//std::size_t size = CreateRawDatagram(PacketHeader::NORMAL, PacketType::OK, payload, buffer);
			//_socket.send(buffer, size, job.sender.value(), job.port);
			SendDatagram(_socket, PacketHeader::NORMAL, PacketType::OK, "", job.sender.value(), job.port);
		}
		else 
		{
			char buffer[64];
			std::string payload = _lastestVersion;
			//std::size_t size = CreateRawDatagram(PacketHeader::NORMAL, PacketType::UPDATE, payload, buffer);
			//_socket.send(buffer, size, job.sender.value(), job.port);
			SendDatagram(_socket, PacketHeader::NORMAL, PacketType::UPDATE, payload, job.sender.value(), job.port);
			SendFile(job.sender.value(), job.port);
		}	
		});

	_dispatcher.Start();

	while (running) 
	{
		char buffer[1024];
		std::size_t received;
		std::optional<sf::IpAddress> sender = std::nullopt;
		unsigned short port;

		if (_socket.receive(buffer, sizeof(buffer), received, sender, port) == sf::Socket::Status::Done)
		{
			RawPacketJob job;
			if (ParseRawDatagram(buffer, received, job, sender.value(), port))
				_dispatcher.EnqueuePacket(job);
		}
	}
	_dispatcher.Stop();
}

void VersionChecker::SendFile(sf::IpAddress address, unsigned short port)
{
	std::ifstream file(_mapFilePath);
	if (!file.is_open()) {
		WriteConsole("[VERSION_CHECKER] Could not open map file: ", _mapFilePath);
		return;
	}

	std::ostringstream fullMap;
	std::string line;
	while (std::getline(file, line)) {
		fullMap << line << "\n";
	}

	SendDatagram(_socket, PacketHeader::NORMAL, PacketType::UPDATE_MAP, fullMap.str(), address, port);

	WriteConsole("[VERSION_CHECKER] Map sent as single datagram to ", address, ":", port);
}

