#include "network_session.h"

#ifndef ENET_IPV4_ONLY
#define ENET_IPV4_ONLY 1
#endif
#include <enet.h>

namespace Chained
{

	NetworkSession::~NetworkSession()
	{
		if (m_Role != Role::Offline)
		{
			CH_CORE_WARN("NetworkSession: Destructor called before explicit Shutdown().");
			Shutdown();
		}
	}

	NetworkError NetworkSession::Initialize()
	{
		if (enet_initialize() != 0)
		{
			CH_CORE_ERROR("NetworkSession: Failed to initialize ENet.");
			return NetworkError::ENetInitFailed;
		}

		m_Role = Role::Offline;
		CH_CORE_INFO("NetworkSession: ENet initialized successfully.");
		return NetworkError::None;
	}

	void NetworkSession::Shutdown()
	{
		m_ShuttingDown.store(true, std::memory_order_release);

		Disconnect();

		if (m_Host)
		{
			enet_host_destroy(m_Host);
			m_Host = nullptr;
		}

		m_Role = Role::Offline;
		m_Port = 0;
		m_ServerConnection = kInvalidPeerHandle;
		m_PeerMap.clear();

		enet_deinitialize();

		CH_CORE_INFO("NetworkSession: Shutdown complete.");
	}

	NetworkError NetworkSession::HostGame(uint16_t port, int maxClients)
	{
		if (m_Role != Role::Offline)
		{
			CH_CORE_ERROR("NetworkSession: Cannot host — already in {} mode.", static_cast<int>(m_Role));
			return NetworkError::AlreadyConnected;
		}

		if (m_Host)
		{
			enet_host_destroy(m_Host);
			m_Host = nullptr;
		}

		ENetAddress address;
		address.host = ENET_HOST_ANY;
		address.port = port;

		m_Host = enet_host_create(&address, maxClients, 2, 0, 0);
		if (!m_Host)
		{
			CH_CORE_ERROR("NetworkSession: Failed to create ENet host on port {}.", port);
			return NetworkError::CreateHostFailed;
		}

		m_MaxClients = maxClients;
		m_Port = port;
		m_Role = Role::Host;

		CH_CORE_INFO("NetworkSession: Hosting on port {} (max {} clients).", m_Port, maxClients);
		return NetworkError::None;
	}

	NetworkError NetworkSession::ConnectTo(const std::string& ip, uint16_t port)
	{
		if (m_Role != Role::Offline)
		{
			CH_CORE_ERROR("NetworkSession: Cannot connect — already in {} mode.", static_cast<int>(m_Role));
			return NetworkError::AlreadyConnected;
		}

		if (m_Host)
		{
			enet_host_destroy(m_Host);
			m_Host = nullptr;
		}

		m_Host = enet_host_create(nullptr, 1, 2, 0, 0);
		if (!m_Host)
		{
			CH_CORE_ERROR("NetworkSession: Failed to create ENet client host.");
			return NetworkError::CreateHostFailed;
		}

		ENetAddress address;
		enet_address_set_host(&address, ip.c_str());
		address.port = port;

		m_ServerPeer = enet_host_connect(m_Host, &address, 2, 0);
		if (!m_ServerPeer)
		{
			CH_CORE_ERROR("NetworkSession: Failed to connect to {}:{}.", ip, port);
			enet_host_destroy(m_Host);
			m_Host = nullptr;
			return NetworkError::ConnectFailed;
		}

		m_MaxClients = 1;
		m_Role = Role::Client;
		m_ServerConnection = 0;
		CH_CORE_INFO("NetworkSession: Connecting to {}:{}...", ip, port);
		return NetworkError::None;
	}

	void NetworkSession::Disconnect()
	{
		if (m_ServerPeer)
		{
			enet_peer_disconnect_now(m_ServerPeer, 0);
			m_ServerPeer = nullptr;
		}

		if (m_Host)
		{
			if (m_Role == Role::Host)
			{
				for (auto& [idx, peer] : m_PeerMap)
				{
					enet_peer_disconnect_now(peer, 0);
				}
			}
			enet_host_destroy(m_Host);
			m_Host = nullptr;
		}

		m_PeerMap.clear();
		m_Role = Role::Offline;
		m_ServerConnection = kInvalidPeerHandle;

		CH_CORE_INFO("NetworkSession: Disconnected.");
	}

	void NetworkSession::ForceDisconnect()
	{
		Disconnect();
	}

	void NetworkSession::Update(float /*dt*/)
	{
		if (m_Role == Role::Offline || !m_Host)
		{
			return;
		}

		ProcessEvents();
	}

	void NetworkSession::ProcessEvents()
	{
		ENetEvent event;
		while (enet_host_service(m_Host, &event, 0) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT: {
				if (m_Role == Role::Host)
				{
					int clientIndex = 0;
					while (m_PeerMap.find(clientIndex) != m_PeerMap.end())
					{
						++clientIndex;
					}
					m_PeerMap[clientIndex] = event.peer;
					event.peer->data = reinterpret_cast<void*>(static_cast<intptr_t>(clientIndex));
					if (m_ConnectionCallback)
					{
						m_ConnectionCallback(clientIndex, 0);
					}
				}
				else if (m_Role == Role::Client)
				{
					CH_CORE_INFO("NetworkSession: Connected to server.");
				}
				break;
			}

			case ENET_EVENT_TYPE_DISCONNECT: {
				if (m_Role == Role::Host)
				{
					int clientIndex = static_cast<int>(reinterpret_cast<intptr_t>(event.peer->data));

					for (auto it = m_PeerMap.begin(); it != m_PeerMap.end(); ++it)
					{
						if (it->second == event.peer)
						{
							m_PeerMap.erase(it);
							break;
						}
					}

					if (m_DisconnectionCallback)
					{
						m_DisconnectionCallback(clientIndex, 0);
					}
				}
				else if (m_Role == Role::Client)
				{
					CH_CORE_INFO("NetworkSession: Disconnected from server.");
					m_ServerPeer = nullptr;
					m_Role = Role::Offline;
				}
				break;
			}

			case ENET_EVENT_TYPE_RECEIVE: {
				int peerIndex = -1;
				if (m_Role == Role::Host)
				{
					peerIndex = static_cast<int>(reinterpret_cast<intptr_t>(event.peer->data));
				}
				if (m_EventCallback)
				{
					m_EventCallback(peerIndex, MessageType_Count, event.packet->data, event.packet->dataLength);
				}
				enet_packet_destroy(event.packet);
				break;
			}

			default:
				break;
			}
		}
	}

	void NetworkSession::MarkClientConnected(int /*peerIndex*/)
	{
	}

	void NetworkSession::MarkClientDisconnected(int /*peerIndex*/)
	{
	}

	std::string NetworkSession::GetListenAddress() const
	{
		if (!m_Host || m_Role != Role::Host)
		{
			return {};
		}

		return "127.0.0.1:" + std::to_string(m_Port);
	}

	std::string NetworkSession::GetPublicAddress() const
	{
		if (!m_PublicAddressFetched.load(std::memory_order_acquire))
		{
			return "Fetching...";
		}
		return m_CachedPublicAddress;
	}

	ENetPeer* NetworkSession::GetPeerForClient(int clientIndex) const
	{
		auto it = m_PeerMap.find(clientIndex);
		return it != m_PeerMap.end() ? it->second : nullptr;
	}

} // namespace Chained
