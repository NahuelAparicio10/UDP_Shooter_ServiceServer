#include "VersionChecker.h"

VersionChecker::VersionChecker() 
{ 
	_lastestVersion = GetLocalVersion();
}

VersionChecker::~VersionChecker() { _socket.unbind(); }

// -- Bind to a specific port

bool VersionChecker::InitializeSocket()
{
	if (_socket.bind(VersionCheckerServerPort) != sf::Socket::Status::Done)
	{
		WriteConsole("[VERSION_CHECKER] Failed to bind to port ", VersionCheckerServerPort);
		return false;
	}

	WriteConsole("[VERSION_CHECKER] Listening on port ", VersionCheckerServerPort);
	
	return true;
}

// -- Function that gets and reads the local version file

std::string VersionChecker::GetLocalVersion()
{
	std::ifstream file(VersionFile);

	if (!file.is_open())
	{
		std::cerr << "[VERSION_CHECKER] Couldn't open version file, assuming 0.0" << std::endl;
		return "0.0";
	}

	std::string version;
	std::getline(file, version);
	return version;
}

// -- Main Loop of the thread checking incoming packages

void VersionChecker::Run(std::atomic<bool>& running)
{
	if (!InitializeSocket()) return;

	// - Subcribes the following code in case VERSION packet arrives 
	// - The code does: checks if versions are the same, in case not send the new version and the new map
	_dispatcher.RegisterHandler(PacketType::VERSION, [this](const RawPacketJob& job) {
		std::string version = job.content;

		// - If the client version and server version match send OK to the client
		
		if (version == _lastestVersion) 
		{
			char buffer[64];
			SendDatagram(_socket, PacketHeader::NORMAL, PacketType::OK, "", job.sender.value(), job.port);
		}
		// - If the clients version and server version doesn't match sends UPDATE to the client and sends the new map file
		else 
		{
			char buffer[64];
			std::string payload = _lastestVersion;
			SendDatagram(_socket, PacketHeader::NORMAL, PacketType::UPDATE, payload, job.sender.value(), job.port);
			SendFile(job.sender.value(), job.port);
		}	
	});

	_dispatcher.Start();

	// -- While port is on, receiving client datagrams and queu them
	
	while (running) 
	{
		char buffer[1024];
		std::size_t received;
		std::optional<sf::IpAddress> sender = std::nullopt;
		unsigned short port;

		if (_socket.receive(buffer, sizeof(buffer), received, sender, port) == sf::Socket::Status::Done)
		{
			// - When we recieve a packet we send it to enqueue to the dispatcher 
			RawPacketJob job;
			if (ParseRawDatagram(buffer, received, job, sender.value(), port))
			{
				_dispatcher.EnqueuePacket(job);
			}
		}
	}
	_dispatcher.Stop();
}

// -- Function that sends to client the new map version

void VersionChecker::SendFile(sf::IpAddress address, unsigned short port)
{
	std::ifstream file(MapFile);
	if (!file.is_open()) {
		WriteConsole("[VERSION_CHECKER] Could not open map file: ", MapFile);
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

