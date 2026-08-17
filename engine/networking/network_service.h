#ifndef CH_NETWORK_SERVICE_H
#define CH_NETWORK_SERVICE_H

#include "net_packet.h"
#include "upnp_port_mapper.h"
#include "engine/core/service.h"

// Force IPv4-only ENet sockets — see network_service.cpp for rationale.
#ifndef ENET_IPV4_ONLY
#define ENET_IPV4_ONLY 1
#endif
#include <enet.h>

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Chained
{
	using NetworkPeerHandle = int;
	constexpr NetworkPeerHandle kInvalidPeerHandle = -1;

	enum class Role : uint8_t
	{
		Offline = 0,
		Host,
		Client,
		HostAndClient,
	};

	struct ChatMessagePacket
	{
		uint64_t SenderNetworkID = 0;
		std::string SenderName;
		std::string Message;
	};

	class Network : public Service
	{
	public:
		using PacketCallback = std::function<void(int, MessageType, const uint8_t*, size_t)>;

		Network() = default;
		~Network() override;

		void Initialize() override;
		void Shutdown() override;

		void HostGame(uint16_t port, int maxClients);
		void ConnectTo(const std::string& ip, uint16_t port);
		void Disconnect();

		void Update(float dt);

		void SendPacket(int clientIndex, MessageType type, const void* data, size_t len, bool reliable = true);
		void BroadcastPacket(MessageType type, bool reliable, const std::function<void(ByteWriter&)>& populate);
		void SendToServer(MessageType type, const void* data, size_t len, bool reliable = true);

		void BroadcastSceneChange(const char* scenePath);

		void SetPacketCallback(PacketCallback callback);
		void ClearPacketCallback();

		std::string GetListenAddress();
		std::string GetPublicAddress();

		bool HasPendingSceneChange() const
		{
			return !m_PendingSceneChange.empty();
		}
		const std::string& GetPendingSceneChange() const
		{
			return m_PendingSceneChange;
		}
		void ClearPendingSceneChange()
		{
			m_PendingSceneChange.clear();
		}
		void SetPendingSceneChange(const std::string& path)
		{
			m_PendingSceneChange = path;
		}

		void SendPlayerInfoToHost(const char* name, uint8_t skinIndex);
		void BroadcastPlayerList();
		void SendChatMessage(const char* message);
		void StorePendingChatMessage(const ChatMessagePacket& pkt);
		bool HasPendingChatMessages() const
		{
			return !m_PendingChatMessages.empty();
		}
		const std::vector<ChatMessagePacket>& GetPendingChatMessages() const
		{
			return m_PendingChatMessages;
		}
		void ClearPendingChatMessages()
		{
			m_PendingChatMessages.clear();
		}

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

		size_t GetClientCount() const;
		std::vector<int> GetClients() const;
		int GetServerConnection() const
		{
			return m_ServerConnection;
		}
		uint16_t GetPort() const
		{
			return m_Port;
		}

		uint64_t GetLocalNetworkID() const
		{
			return m_LocalNetworkID;
		}
		void SetLocalNetworkID(uint64_t id)
		{
			m_LocalNetworkID = id;
		}

		void SetLocalPlayerInfo(const char* name, uint8_t skinIndex);

		uint64_t GetNetworkIDForConnection(int clientIndex) const;

		const std::vector<PlayerNetInfo>& GetPlayerList() const
		{
			return m_PlayerList;
		}
		std::vector<PlayerNetInfo>& GetPlayerListMutable()
		{
			return m_PlayerList;
		}

		int GetMaxClients() const
		{
			return m_MaxClients;
		}

		bool IsClientConnected(int clientIndex) const;

	private:
		void ProcessEvents();
		void OnClientConnected(int clientIndex);
		void OnClientDisconnected(int clientIndex);
		uint64_t GenerateClientId();

		void HandlePacket(int peerIndex, const uint8_t* data, size_t len);
		void SendRaw(int peerIndex, MessageType type, const void* payload, size_t payloadLen, bool reliable);

		Role m_Role = Role::Offline;
		ENetHost* m_Host = nullptr;
		ENetPeer* m_ServerPeer = nullptr;
		std::unordered_map<int, ENetPeer*> m_PeerMap;
		int m_MaxClients = 0;

		int m_ServerConnection = kInvalidPeerHandle;
		uint16_t m_Port = 0;

		std::string m_CachedPublicAddress;
		std::atomic<bool> m_PublicAddressFetched{false};
		std::atomic<bool> m_ShuttingDown{false};

		PacketCallback m_PacketCallback;
		std::string m_PendingSceneChange;

		std::vector<PlayerNetInfo> m_PlayerList;
		std::vector<ChatMessagePacket> m_PendingChatMessages;
		std::unordered_map<int, uint64_t> m_ClientIndexToNetworkID;
		uint64_t m_LocalNetworkID = 0;
		std::string m_LocalPlayerName;
		uint8_t m_LocalSkinIndex = 0;
		static constexpr uint64_t HostNetworkID = 1;
		uint64_t m_NextNetworkID = 2;

		// Crypto state
		uint8_t m_SessionKey[32] = {};
		bool m_CryptoEnabled = false;
		std::unordered_map<int, uint64_t> m_SendCounters;
		std::unordered_map<int, uint64_t> m_RecvCounters;

		UpnpPortMapper m_UpnpMapper;
	};

} // namespace Chained

#endif /* CH_NETWORK_SERVICE_H */
