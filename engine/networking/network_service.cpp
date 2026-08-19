#include "network_service.h"

namespace Chained
{
	Network::~Network()
	{
		if (m_Session.IsConnected())
		{
			CH_CORE_WARN("Network: Destructor called before explicit Shutdown().");
			Shutdown();
		}
	}

	void Network::Initialize()
	{
		NetworkError err = m_Session.Initialize();
		if (err != NetworkError::None)
		{
			SetEnabled(false);
			return;
		}

		static const uint8_t kSessionKey[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
												0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
												0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
		m_Transport.SetSessionKey(kSessionKey);
		m_Transport.SetCryptoEnabled(false);
		m_Transport.SetSession(&m_Session);

		m_Session.SetConnectionCallback(
			[this](int peerIndex, uint64_t networkID) { OnClientConnectedInternal(peerIndex, networkID); });
		m_Session.SetDisconnectionCallback(
			[this](int peerIndex, uint64_t networkID) { OnClientDisconnectedInternal(peerIndex, networkID); });
		m_Session.SetEventCallback([this](int peerIndex, MessageType type, const uint8_t* data, size_t len) {
			if (type == MessageType_Count)
			{
				m_Transport.HandleIncomingPacket(peerIndex, data, len);
			}
		});

		CH_CORE_INFO("Network: Initialized.");
	}

	void Network::Shutdown()
	{
		uint16_t portToUnmap = m_Session.GetPort();

		Disconnect();

		if (m_UpnpMapper.IsAvailable() && portToUnmap != 0)
		{
			m_UpnpMapper.RemoveMapping(portToUnmap, "UDP");
			m_UpnpMapper.Shutdown();
		}

		m_Session.Shutdown();

		CH_CORE_INFO("Network: Shutdown complete.");
	}

	void Network::HostGame(uint16_t port, int maxClients)
	{
		NetworkError err = m_Session.HostGame(port, maxClients);
		if (err != NetworkError::None)
		{
			CH_CORE_ERROR("Network: Failed to host (error={}).", static_cast<int>(err));
			return;
		}

		m_PlayerManager.Reset();
		m_PlayerManager.SetHostNetworkID(HostNetworkID);
		m_PlayerManager.AddHostSelf(HostNetworkID, m_PlayerManager.GetLocalPlayerName(),
									m_PlayerManager.GetLocalSkinIndex());

		if (!m_UpnpMapper.IsAvailable())
		{
			m_UpnpMapper.Initialize();
		}
		if (m_UpnpMapper.IsAvailable())
		{
			m_UpnpMapper.AddMapping(port, "UDP", "ChainedDecos");
			std::string wan = m_UpnpMapper.GetPublicIP();
			CH_CORE_INFO("Network: UPnP mapping added.");
		}
		else
		{
			CH_CORE_WARN("Network: UPnP unavailable — players must forward port {} manually.", port);
		}

		CH_CORE_INFO("Network: Hosting on port {} (max {} clients).", port, maxClients);
	}

	void Network::ConnectTo(const std::string& ip, uint16_t port)
	{
		NetworkError err = m_Session.ConnectTo(ip, port);
		if (err != NetworkError::None)
		{
			CH_CORE_ERROR("Network: Failed to connect (error={}).", static_cast<int>(err));
			return;
		}

		m_PlayerManager.Reset();
		CH_CORE_INFO("Network: Connecting to {}:{}...", ip, port);
	}

	void Network::Disconnect()
	{
		m_Session.Disconnect();
		m_Transport.ClearPacketCallback();
		m_Transport.ResetCounters();
		m_PlayerManager.Reset();
		m_PendingSceneChange.clear();
		CH_CORE_INFO("Network: Disconnected.");
	}

	void Network::Update(float dt)
	{
		m_Session.Update(dt);
	}

	// ── Sending ──────────────────────────────────────────────────────────

	void Network::SendPacket(int clientIndex, MessageType type, const void* data, size_t len, bool reliable)
	{
		m_Transport.SendPacket(clientIndex, type, data, len, reliable);
	}

	void Network::BroadcastPacket(MessageType type, bool reliable, const std::function<void(ByteWriter&)>& populate)
	{
		m_Transport.BroadcastPacket(type, reliable, populate);
	}

	void Network::SendToServer(MessageType type, const void* data, size_t len, bool reliable)
	{
		m_Transport.SendToServer(type, data, len, reliable);
	}

	void Network::BroadcastSceneChange(const char* scenePath)
	{
		m_Transport.BroadcastSceneChange(scenePath);
	}

	// ── Packet callback ──────────────────────────────────────────────────

	void Network::SetPacketCallback(PacketCallback callback)
	{
		m_Transport.SetPacketCallback(std::move(callback));
	}

	void Network::ClearPacketCallback()
	{
		m_Transport.ClearPacketCallback();
	}

	// ── Address ──────────────────────────────────────────────────────────

	std::string Network::GetListenAddress()
	{
		return m_Session.GetListenAddress();
	}

	std::string Network::GetPublicAddress()
	{
		return m_Session.GetPublicAddress();
	}

	// ── Player management ────────────────────────────────────────────────

	void Network::SetLocalPlayerInfo(const char* name, uint8_t skinIndex)
	{
		m_PlayerManager.SetLocalPlayerInfo(name, skinIndex);

		if (!m_Session.IsHost())
		{
			return;
		}

		auto& playerList = m_PlayerManager.GetPlayerListMutable();
		auto self = std::find_if(playerList.begin(), playerList.end(),
								 [](const PlayerNetInfo& p) { return p.NetworkID == HostNetworkID; });

		if (self != playerList.end())
		{
			self->Name = name ? name : "Host";
			self->SkinIndex = skinIndex;
			BroadcastPlayerList();
		}
	}

	void Network::SendPlayerInfoToHost(const char* name, uint8_t skinIndex)
	{
		m_Transport.SendPlayerInfoToHost(name, skinIndex, m_PlayerManager.GetLocalNetworkID());
	}

	void Network::BroadcastPlayerList()
	{
		m_Transport.BroadcastPlayerList(m_PlayerManager.GetPlayerList());
	}

	void Network::SendChatMessage(const char* message)
	{
		m_Transport.SendChatMessage(message, m_PlayerManager.GetLocalNetworkID(), m_PlayerManager.GetLocalPlayerName());
	}

	void Network::StorePendingChatMessage(const ChatMessagePacket& pkt)
	{
		m_Transport.StorePendingChatMessage(pkt);
	}

	// ── Client info ──────────────────────────────────────────────────────

	size_t Network::GetClientCount() const
	{
		return m_PlayerManager.GetClientCount();
	}

	std::vector<int> Network::GetClients() const
	{
		return m_PlayerManager.GetClients();
	}

	bool Network::IsClientConnected(int clientIndex) const
	{
		return m_Session.GetPeerForClient(clientIndex) != nullptr;
	}

	// ── Connection callbacks ─────────────────────────────────────────────

	void Network::OnClientConnectedInternal(int clientIndex, uint64_t /*networkID*/)
	{
		m_PlayerManager.OnClientConnected(clientIndex);
		const uint64_t assignedID = m_PlayerManager.GetNetworkIDForConnection(clientIndex);

		PlayerAssignMessage assignMsg;
		assignMsg.NetworkID = assignedID;
		ByteWriter w;
		assignMsg.Encode(w);
		m_Transport.SendPacket(clientIndex, MessageType_PlayerAssign, w.Data().data(), w.Data().size(), true);

		BroadcastPlayerList();
		CH_CORE_INFO("Network: Client connected (index={}, netID={}).", clientIndex, assignedID);
	}

	void Network::OnClientDisconnectedInternal(int clientIndex, uint64_t /*networkID*/)
	{
		m_PlayerManager.OnClientDisconnected(clientIndex);
		BroadcastPlayerList();
	}

} // namespace Chained
