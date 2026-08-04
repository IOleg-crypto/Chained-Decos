#define ENET_IMPLEMENTATION
#include "network_service.h"
#include "engine/core/log.h"

#include <algorithm>
#include <cstring>

namespace Chained
{

	Network::~Network()
	{
		if (m_Host)
		{
			CH_CORE_WARN("Network: Destructor called before explicit Shutdown().");
			Shutdown();
		}
	}

	void Network::Initialize()
	{
		if (enet_initialize() != 0)
		{
			CH_CORE_ERROR("Network: Failed to initialize ENet.");
			SetEnabled(false);
			return;
		}
		CH_CORE_INFO("Network: Initialized successfully.");
	}

	void Network::Shutdown()
	{
		Disconnect();

		if (m_Host)
		{
			enet_host_destroy(m_Host);
			m_Host = nullptr;
		}

		m_Clients.clear();
		m_ServerPeer = nullptr;
		m_Role = Role::Offline;

		enet_deinitialize();
		CH_CORE_INFO("Network: Shutdown complete.");
	}

	void Network::HostGame(uint16_t port, int maxClients)
	{
		ENetAddress address;
		address.host = ENET_HOST_ANY;
		address.port = port;

		m_Host = enet_host_create(&address, maxClients, 2, 0, 0);
		if (!m_Host)
		{
			CH_CORE_ERROR("Network: Failed to create ENet host on port {}.", port);
			SetEnabled(false);
			return;
		}

		m_Role = Role::Host;
		CH_CORE_INFO("Network: Hosting on port {} (max {} clients).", port, maxClients);
	}

	void Network::ConnectTo(const std::string& ip, uint16_t port)
	{
		m_Host = enet_host_create(nullptr, 1, 2, 0, 0);
		if (!m_Host)
		{
			CH_CORE_ERROR("Network: Failed to create ENet client host.");
			SetEnabled(false);
			return;
		}

		ENetAddress address;
		enet_address_set_host(&address, ip.c_str());
		address.port = port;

		m_ServerPeer = enet_host_connect(m_Host, &address, 2, 0);
		if (!m_ServerPeer)
		{
			CH_CORE_ERROR("Network: Failed to initiate connection to {}:{}.", ip, port);
			SetEnabled(false);
			return;
		}

		m_Role = Role::Client;
		CH_CORE_INFO("Network: Connecting to {}:{}.", ip, port);
	}

	void Network::Disconnect()
	{
		if (m_Host && m_ServerPeer)
		{
			enet_peer_disconnect(m_ServerPeer, 0);
			m_ServerPeer = nullptr;
		}

		for (auto* peer : m_Clients)
		{
			enet_peer_disconnect_now(peer, 0);
		}
		m_Clients.clear();

		if (m_Host)
		{
			enet_host_flush(m_Host);
		}
	}

	void Network::Update(float /*dt*/)
	{
		if (!m_Host)
		{
			return;
		}

		ENetEvent event;
		while (enet_host_service(m_Host, &event, 0) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				if (m_Role == Role::Host)
				{
					m_Clients.push_back(event.peer);
					CH_CORE_INFO("Network: Client connected (total: {}).", m_Clients.size());
				}
				else
				{
					CH_CORE_INFO("Network: Connected to server.");
				}
				break;

			case ENET_EVENT_TYPE_RECEIVE:
				// TODO: process incoming packets by event.packet->data
				enet_packet_destroy(event.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				if (m_Role == Role::Host)
				{
					auto it = std::find(m_Clients.begin(), m_Clients.end(), event.peer);
					if (it != m_Clients.end())
					{
						m_Clients.erase(it);
						CH_CORE_INFO("Network: Client disconnected (total: {}).", m_Clients.size());
					}
				}
				else
				{
					CH_CORE_INFO("Network: Disconnected from server.");
					m_ServerPeer = nullptr;
				}
				break;

			default:
				break;
			}
		}
	}

	void Network::SendPacket(ENetPeer* peer, PacketType type, const void* data, size_t size)
	{
		if (!m_Host || !peer)
		{
			return;
		}

		size_t totalSize = sizeof(PacketType) + size;
		ENetPacket* packet = enet_packet_create(nullptr, totalSize, ENET_PACKET_FLAG_RELIABLE);

		auto* raw = static_cast<uint8_t*>(packet->data);
		std::memcpy(raw, &type, sizeof(PacketType));
		if (size > 0 && data)
		{
			std::memcpy(raw + sizeof(PacketType), data, size);
		}

		enet_peer_send(peer, 0, packet);
	}

	void Network::BroadcastPacket(PacketType type, const void* data, size_t size)
	{
		for (auto* peer : m_Clients)
		{
			SendPacket(peer, type, data, size);
		}
	}

	void Network::SendToServer(PacketType type, const void* data, size_t size)
	{
		if (!m_Host || !m_ServerPeer)
		{
			return;
		}
		SendPacket(m_ServerPeer, type, data, size);
	}

} // namespace Chained
