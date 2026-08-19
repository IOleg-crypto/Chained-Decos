#ifndef CH_NETWORK_SESSION_H
#define CH_NETWORK_SESSION_H

#include "network_types.h"
#include "net_packet.h"

#ifndef ENET_IPV4_ONLY
#define ENET_IPV4_ONLY 1
#endif
#include <enet.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace Chained
{

	class NetworkSession
	{
	public:
		using EventCallback = std::function<void(int peerIndex, MessageType type, const uint8_t* data, size_t len)>;
		using ConnectionCallback = std::function<void(int peerIndex, uint64_t networkID)>;
		using DisconnectionCallback = std::function<void(int peerIndex, uint64_t networkID)>;

		NetworkSession() = default;
		~NetworkSession();

		NetworkSession(const NetworkSession&) = delete;
		NetworkSession& operator=(const NetworkSession&) = delete;

		NetworkError Initialize();
		void Shutdown();

		NetworkError HostGame(uint16_t port, int maxClients);
		NetworkError ConnectTo(const std::string& ip, uint16_t port);
		void Disconnect();
		void ForceDisconnect();

		void Update(float dt);

		Role GetRole() const
		{
			return m_Role;
		}
		bool IsHost() const
		{
			return m_Role == Role::Host || m_Role == Role::HostAndClient;
		}
		bool IsClient() const
		{
			return m_Role == Role::Client || m_Role == Role::HostAndClient;
		}
		bool IsConnected() const
		{
			return m_Role != Role::Offline;
		}

		int GetMaxClients() const
		{
			return m_MaxClients;
		}
		uint16_t GetPort() const
		{
			return m_Port;
		}
		std::string GetListenAddress() const;
		std::string GetPublicAddress() const;

		void SetEventCallback(EventCallback cb)
		{
			m_EventCallback = std::move(cb);
		}
		void SetConnectionCallback(ConnectionCallback cb)
		{
			m_ConnectionCallback = std::move(cb);
		}
		void SetDisconnectionCallback(DisconnectionCallback cb)
		{
			m_DisconnectionCallback = std::move(cb);
		}

		void SetMaxClients(int max)
		{
			m_MaxClients = max;
		}

		void MarkClientConnected(int peerIndex);
		void MarkClientDisconnected(int peerIndex);

		ENetHost* GetHost() const
		{
			return m_Host;
		}
		ENetPeer* GetServerPeer() const
		{
			return m_ServerPeer;
		}
		ENetPeer* GetPeerForClient(int clientIndex) const;
		const std::unordered_map<int, ENetPeer*>& GetPeerMap() const
		{
			return m_PeerMap;
		}

		void SetServerConnection(int conn)
		{
			m_ServerConnection = conn;
		}
		int GetServerConnection() const
		{
			return m_ServerConnection;
		}

		std::atomic<bool> m_ShuttingDown{false};

	private:
		void ProcessEvents();

		Role m_Role = Role::Offline;
		ENetHost* m_Host = nullptr;
		ENetPeer* m_ServerPeer = nullptr;
		int m_MaxClients = 0;
		uint16_t m_Port = 0;
		int m_ServerConnection = kInvalidPeerHandle;

		std::string m_CachedPublicAddress;
		std::atomic<bool> m_PublicAddressFetched{false};

		EventCallback m_EventCallback;
		ConnectionCallback m_ConnectionCallback;
		DisconnectionCallback m_DisconnectionCallback;

		std::unordered_map<int, ENetPeer*> m_PeerMap;
	};

} // namespace Chained

#endif // CH_NETWORK_SESSION_H
