#include "PacketDispatcher.h"
#include "ConsoleUtils.h"
PacketDispatcher::PacketDispatcher() : _running(false)
{
}

PacketDispatcher::~PacketDispatcher()
{
	Stop();
}

// -- Enqueue a packet to the queue depending on his header

void PacketDispatcher::EnqueuePacket(const RawPacketJob& job)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (job.headerMask & PacketHeader::URGENT) 
    {
        _queueUrgent.push(job);
    }
    else if (job.headerMask & PacketHeader::CRITIC) 
    {
        _queueCritical.push(job);
    }
    else 
    {
        _queueNormal.push(job);
    }
}

// -- Registers a action handler in case it triggers

void PacketDispatcher::RegisterHandler(PacketType type, std::function<void(const RawPacketJob&)> handler) {	_handlers[type] = handler; }

void PacketDispatcher::Start()
{
	_running = true;
	_dispatchThread = std::thread(&PacketDispatcher::DispatchLoop, this);
}

void PacketDispatcher::Stop()
{
	_running = false;
	if (_dispatchThread.joinable()) _dispatchThread.join();
}

//-- Triggers the register handler depending on the package in queue by priority (urgent, critic, normal)

void PacketDispatcher::DispatchLoop()
{
    while (_running)
    {
        RawPacketJob job;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_queueUrgent.empty()) 
            {
                job = _queueUrgent.front();
                _queueUrgent.pop();
            }
            else if (!_queueCritical.empty()) 
            {
                job = _queueCritical.front();
                _queueCritical.pop();
            }
            else if (!_queueNormal.empty()) 
            {
                job = _queueNormal.front();
                _queueNormal.pop();
            }
            else {
                continue;
            }
        }

        auto it = _handlers.find(job.type);
        
        if (it != _handlers.end()) 
        {
            // - Triggers all sucribers from functional event
            it->second(job);
        }
    }
}
