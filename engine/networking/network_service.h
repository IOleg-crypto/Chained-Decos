#ifndef CH_NETWORK_SERVICE_H
#define CH_NETWORK_SERVICE_H

#include "net_packet.h"
#include "network_types.h"
#include "network_session.h"
#include "network_transport.h"
#include "network_player_manager.h"
#include "upnp_port_mapper.h"
#include "engine/core/service.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Chained
{
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
			return m_Transport.HasPendingChatMessages();
		}
		const std::vector<ChatMessagePacket>& GetPendingChatMessages() const
		{
			return m_Transport.GetPendingChatMessages();
		}
		void ClearPendingChatMessages()
		{
			m_Transport.ClearPendingChatMessages();
		}

		Role GetRole() const
		{
			return m_Session.GetRole();
		}
		bool IsHost() const
		{
			return m_Session.IsHost();
		}
		bool IsClient() const
		{
			return m_Session.IsClient();
		}
		bool IsConnected() const
		{
			return m_Session.IsConnected();
		}
		bool IsFullyConnected() const
		{
			return m_Session.IsFullyConnected();
		}

		size_t GetClientCount() const;
		std::vector<int> GetClients() const;
		int GetServerConnection() const
		{
			return m_Session.GetServerConnection();
		}
		uint16_t GetPort() const
		{
			return m_Session.GetPort();
		}

		uint64_t GetLocalNetworkID() const
		{
			return m_PlayerManager.GetLocalNetworkID();
		}
		void SetLocalNetworkID(uint64_t id)
		{
			m_PlayerManager.SetLocalNetworkID(id);
		}

		void SetLocalPlayerInfo(const char* name, uint8_t skinIndex);

		uint64_t GetNetworkIDForConnection(int clientIndex) const
		{
			return m_PlayerManager.GetNetworkIDForConnection(clientIndex);
		}

		const std::vector<PlayerNetInfo>& GetPlayerList() const
		{
			return m_PlayerManager.GetPlayerList();
		}
		std::vector<PlayerNetInfo>& GetPlayerListMutable()
		{
			return m_PlayerManager.GetPlayerListMutable();
		}

		int GetMaxClients() const
		{
			return m_Session.GetMaxClients();
		}
		bool IsClientConnected(int clientIndex) const;
		bool IsUpnpAvailable() const
		{
			return m_UpnpMapper.IsAvailable();
		}
		bool IsFirewallRuleActive() const
		{
			return m_FirewallRuleActive;
		}

		void SetTestMode(bool enabled)
		{
			m_TestMode = enabled;
		}
		bool IsTestMode() const
		{
			return m_TestMode;
		}

		NetworkSession& GetSession()
		{
			return m_Session;
		}
		NetworkTransport& GetTransport()
		{
			return m_Transport;
		}
		NetworkPlayerManager& GetPlayerManager()
		{
			return m_PlayerManager;
		}

	private:
		void OnClientConnectedInternal(int clientIndex, uint64_t networkID);
		void OnClientDisconnectedInternal(int clientIndex, uint64_t networkID);

		NetworkSession m_Session;
		NetworkTransport m_Transport;
		NetworkPlayerManager m_PlayerManager;

		std::string m_PendingSceneChange;
		UpnpPortMapper m_UpnpMapper;

		std::string m_CachedPublicIP;
		std::mutex m_PublicIPMutex;
		bool m_FirewallRuleActive = false;
		bool m_TestMode = false;

		static constexpr uint64_t HostNetworkID = 1;
	};

} // namespace Chained

#endif /* CH_NETWORK_SERVICE_H */
