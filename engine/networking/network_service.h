#ifndef CH_NETWORK_SERVICE_H
#define CH_NETWORK_SERVICE_H

#include "net_packet.h"
#include "upnp_port_mapper.h"
#include "engine/core/service.h"
#include <steam/steamnetworkingsockets.h>
#include <atomic>
#include <functional>
#include <future>
#include <string>
#include <unordered_map>
#include <vector>

namespace Chained
{

	enum class Role
	{
		Offline = 0,
		Host = 1,
		Client = 2,
		HostAndClient = 3 // Same role as Host, but also acts as a client connecting to self
	};

	class Network : public Service
	{
	public:
		using PacketCallback = std::function<void(PacketType, const uint8_t*, size_t, HSteamNetConnection)>;

		Network() = default;
		~Network() override;

		void Initialize() override;
		void Shutdown() override;

		void HostGame(uint16_t port, int maxClients);
		void ConnectTo(const std::string& ip, uint16_t port);
		void Disconnect();

		void Update(float dt);

		void SendPacket(HSteamNetConnection conn, PacketType type, const void* data, size_t size, bool reliable = true);
		void BroadcastPacket(PacketType type, const void* data, size_t size, bool reliable = true);
		void SendToServer(PacketType type, const void* data, size_t size, bool reliable = true);
		void BroadcastSceneChange(const char* scenePath);

		// Packet callback — set by NetworkSystem to receive dispatched packets.
		void SetPacketCallback(PacketCallback callback);
		void ClearPacketCallback();

		// Address of the local listen socket (LAN). NOT the public IP.
		std::string GetListenAddress();
		// Public IP:PORT for internet hosting. Fetched asynchronously once after HostGame().
		// Returns "Fetching..." until ready, then e.g. "203.0.113.5:7777".
		std::string GetPublicAddress();

		// Scene change (received from host, client-side).
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

		// Player list management.
		void SendPlayerInfoToHost(const char* name, uint8_t skinIndex);
		void BroadcastPlayerList();
		const std::vector<PlayerNetInfo>& GetPlayerList() const
		{
			return m_PlayerList;
		}
		std::vector<PlayerNetInfo>& GetPlayerListMutable()
		{
			return m_PlayerList;
		}
		void SetLocalPlayerInfo(const char* name, uint8_t skinIndex);
		uint64_t GetLocalNetworkID() const
		{
			return m_LocalNetworkID;
		}
		void SetLocalNetworkID(uint64_t id)
		{
			m_LocalNetworkID = id;
		}

		// NetworkID assigned to a peer at accept time. Returns 0 when unknown.
		uint64_t GetNetworkIDForConnection(HSteamNetConnection conn) const;

		// Chat.
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

		size_t GetClientCount() const
		{
			return m_Clients.size();
		}
		const std::vector<HSteamNetConnection>& GetClients() const
		{
			return m_Clients;
		}
		HSteamNetConnection GetServerConnection() const
		{
			return m_ServerConnection;
		}
		uint16_t GetPort() const
		{
			return m_Port;
		}

	private:
		void ReceiveMessages();

		// GNS callback — dispatched when connection state changes
		void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
		static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);

		Role m_Role = Role::Offline;
		ISteamNetworkingSockets* m_Interface = nullptr;
		HSteamListenSocket m_hListenSocket = k_HSteamListenSocket_Invalid;
		HSteamNetPollGroup m_hPollGroup = k_HSteamNetPollGroup_Invalid;
		HSteamNetConnection m_ServerConnection = k_HSteamNetConnection_Invalid;
		uint16_t m_Port = 0;

		// Cached public IP (fetched asynchronously after HostGame).
		std::string m_CachedPublicAddress;
		std::future<std::string> m_PublicAddressFuture;
		std::atomic<bool> m_PublicAddressFetched{false};
		std::atomic<bool> m_ShuttingDown{false};

		std::vector<HSteamNetConnection> m_Clients;

		PacketCallback m_PacketCallback;
		std::string m_PendingSceneChange;

		// Player list and chat.
		std::vector<PlayerNetInfo> m_PlayerList;
		std::vector<ChatMessagePacket> m_PendingChatMessages;
		// Authoritative peer -> NetworkID mapping (host-side). Assigned at accept
		// time so player info, avatars and disconnects all agree on one identity
		// instead of correlating by list position.
		std::unordered_map<HSteamNetConnection, uint64_t> m_ConnToNetworkID;
		uint64_t m_LocalNetworkID = 0;
		std::string m_LocalPlayerName;
		uint8_t m_LocalSkinIndex = 0;
		// NetworkID 1 is reserved for the host, so peers start at 2.
		static constexpr uint64_t HostNetworkID = 1;
		uint64_t m_NextNetworkID = 2;

		// Single owner of the process-wide GNS library. Only one Network instance
		// exists at a time (registered via ServiceLocator).
		static Network* s_Instance;

		UpnpPortMapper m_UpnpMapper;
	};

} // namespace Chained

#endif /* CH_NETWORK_SERVICE_H */
