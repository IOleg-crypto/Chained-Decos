#include "network_service.h"
#include "net_packet.h"

// Force IPv4-only ENet sockets.
// ENet defaults to AF_INET6 dual-stack, which fails to bind on some Windows
// systems even when IPv6 is technically installed.  Defining ENET_IPV4_ONLY
// switches the socket creation and bind paths to plain AF_INET, which is
// reliable on all supported platforms.
#ifndef ENET_IPV4_ONLY
#define ENET_IPV4_ONLY 1
#endif
#include <enet.h>
#include <sodium.h>

#include <cstring>

#include <random>

namespace Chained
{
	// ── Crypto helpers ───────────────────────────────────────────────────

	static constexpr size_t kCryptoNonceSize = 24;
	static constexpr size_t kCryptoMACSize = 16;

	// Pre-shared session key (same role as yojimbo's kPrivateKey).
	// In production this would be exchanged or derived; here it's compiled in.
	static const uint8_t kSessionKey[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
											0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
											0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};

	static void BuildNonce(uint8_t nonce[kCryptoNonceSize], uint64_t counter)
	{
		std::memset(nonce, 0, kCryptoNonceSize);
		for (int i = 0; i < 8; ++i)
		{
			nonce[i] = static_cast<uint8_t>((counter >> (i * 8)) & 0xFF);
		}
	}

	static bool CryptoEncrypt(const uint8_t* plaintext, size_t ptLen, uint8_t* ciphertext, size_t& ctLen,
							  const uint8_t key[32], uint64_t counter)
	{
		uint8_t nonce[kCryptoNonceSize];
		BuildNonce(nonce, counter);

		unsigned long long actualCtLen = 0;
		if (crypto_aead_xchacha20poly1305_ietf_encrypt(ciphertext, &actualCtLen, plaintext, ptLen, nullptr, 0, nullptr,
													   nonce, key) != 0)
		{
			return false;
		}
		ctLen = static_cast<size_t>(actualCtLen);
		return true;
	}

	static bool CryptoDecrypt(const uint8_t* ciphertext, size_t ctLen, uint8_t* plaintext, size_t& ptLen,
							  const uint8_t key[32], uint64_t counter)
	{
		if (ctLen < kCryptoMACSize)
		{
			return false;
		}

		uint8_t nonce[kCryptoNonceSize];
		BuildNonce(nonce, counter);

		unsigned long long actualPtLen = 0;
		if (crypto_aead_xchacha20poly1305_ietf_decrypt(plaintext, &actualPtLen, nullptr, ciphertext, ctLen, nullptr, 0,
													   nonce, key) != 0)
		{
			return false;
		}
		ptLen = static_cast<size_t>(actualPtLen);
		return true;
	}

	// ── Constants ────────────────────────────────────────────────────────

	static constexpr uint16_t kDefaultPort = 7777;

	// ── Network ──────────────────────────────────────────────────────────

	Network::~Network()
	{
		if (m_Role != Role::Offline)
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

		std::memcpy(m_SessionKey, kSessionKey, 32);
		// NOTE: Crypto is disabled until a proper per-direction nonce scheme is
		// implemented. The current sequential counter is shared between both send
		// and receive directions per peerIndex, causing counter desync as soon as
		// both sides send concurrently, which produces "Decryption failed" spam and
		// dropped packets (leading to crashes in the network system).
		m_CryptoEnabled = false;

		m_Role = Role::Offline;
		CH_CORE_INFO("Network: ENet initialized successfully.");
	}

	void Network::Shutdown()
	{
		m_ShuttingDown.store(true, std::memory_order_release);

		// Capture port before Disconnect() clears it.
		uint16_t portToUnmap = m_Port;

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
		m_SendCounters.clear();
		m_RecvCounters.clear();

		if (m_UpnpMapper.IsAvailable() && portToUnmap != 0)
		{
			m_UpnpMapper.RemoveMapping(portToUnmap, "UDP");
			m_UpnpMapper.Shutdown();
		}

		enet_deinitialize();

		CH_CORE_INFO("Network: Shutdown complete.");
	}

	void Network::HostGame(uint16_t port, int maxClients)
	{
		if (m_Role != Role::Offline)
		{
			CH_CORE_ERROR("Network: Cannot host — already in {} mode.", (int)m_Role);
			return;
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
			CH_CORE_ERROR("Network: Failed to create ENet host on port {}.", port);
			return;
		}

		m_MaxClients = maxClients;
		m_Port = port;
		m_Role = Role::Host;

		m_LocalNetworkID = HostNetworkID;

		PlayerNetInfo self;
		self.NetworkID = HostNetworkID;
		self.Name = m_LocalPlayerName.empty() ? "Host" : m_LocalPlayerName;
		self.SkinIndex = m_LocalSkinIndex;
		self.IsHost = 1;
		self.Ping = 0;
		m_PlayerList.push_back(self);

		// Try UPnP port forwarding so LAN clients can reach us from the internet.
		if (!m_UpnpMapper.IsAvailable())
		{
			m_UpnpMapper.Initialize(); // blocks ~2s; acceptable at session start
		}
		if (m_UpnpMapper.IsAvailable())
		{
			m_UpnpMapper.AddMapping(port, "UDP", "ChainedDecos");
			std::string wan = m_UpnpMapper.GetPublicIP();
			if (!wan.empty())
			{
				m_CachedPublicAddress = wan + ":" + std::to_string(port);
				CH_CORE_INFO("Network: UPnP mapping added — public address: {}", m_CachedPublicAddress);
			}
			else
			{
				m_CachedPublicAddress = "?:" + std::to_string(port);
			}
		}
		else
		{
			CH_CORE_WARN("Network: UPnP unavailable — players must forward port {} manually.", port);
			m_CachedPublicAddress = "127.0.0.1:" + std::to_string(port);
		}
		m_PublicAddressFetched.store(true, std::memory_order_release);

		CH_CORE_INFO("Network: Hosting on port {} (max {} clients).", m_Port, maxClients);
	}

	void Network::ConnectTo(const std::string& ip, uint16_t port)
	{
		if (m_Role != Role::Offline)
		{
			CH_CORE_ERROR("Network: Cannot connect — already in {} mode.", (int)m_Role);
			return;
		}

		if (m_Host)
		{
			enet_host_destroy(m_Host);
			m_Host = nullptr;
		}

		m_Host = enet_host_create(nullptr, 1, 2, 0, 0);
		if (!m_Host)
		{
			CH_CORE_ERROR("Network: Failed to create ENet client host.");
			return;
		}

		ENetAddress address;
		enet_address_set_host(&address, ip.c_str());
		address.port = port;

		m_ServerPeer = enet_host_connect(m_Host, &address, 2, 0);
		if (!m_ServerPeer)
		{
			CH_CORE_ERROR("Network: Failed to connect to {}:{}.", ip, port);
			enet_host_destroy(m_Host);
			m_Host = nullptr;
			return;
		}

		m_MaxClients = 1;
		m_Role = Role::Client;
		m_ServerConnection = 0;
		CH_CORE_INFO("Network: Connecting to {}:{}...", ip, port);
	}

	void Network::Disconnect()
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
		m_ClientIndexToNetworkID.clear();
		m_PlayerList.clear();
		m_PendingChatMessages.clear();
		m_PendingSceneChange.clear();
		m_LocalNetworkID = 0;
		m_NextNetworkID = 2;
		m_SendCounters.clear();
		m_RecvCounters.clear();

		// Reset role so HostGame()/ConnectTo() can be called again cleanly
		m_Role = Role::Offline;
		// Clear stale callback
		m_PacketCallback = nullptr;

		CH_CORE_INFO("Network: Disconnected.");
	}

	uint64_t Network::GenerateClientId()
	{
		std::random_device rd;
		std::mt19937_64 gen(rd());
		return gen();
	}

	void Network::Update(float dt)
	{
		if (m_Role == Role::Offline || !m_Host)
		{
			return;
		}

		ProcessEvents();
	}

	// ── ENet event processing ────────────────────────────────────────────

	void Network::ProcessEvents()
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
					OnClientConnected(clientIndex);
				}
				else if (m_Role == Role::Client)
				{
					CH_CORE_INFO("Network: Connected to server.");
				}
				break;
			}

			case ENET_EVENT_TYPE_DISCONNECT: {
				if (m_Role == Role::Host)
				{
					int clientIndex = static_cast<int>(reinterpret_cast<intptr_t>(event.peer->data));
					OnClientDisconnected(clientIndex);

					for (auto it = m_PeerMap.begin(); it != m_PeerMap.end(); ++it)
					{
						if (it->second == event.peer)
						{
							m_PeerMap.erase(it);
							break;
						}
					}
				}
				else if (m_Role == Role::Client)
				{
					CH_CORE_INFO("Network: Disconnected from server.");
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
				HandlePacket(peerIndex, event.packet->data, event.packet->dataLength);
				enet_packet_destroy(event.packet);
				break;
			}

			default:
				break;
			}
		}
	}

	// ── Packet handling ──────────────────────────────────────────────────

	void Network::HandlePacket(int peerIndex, const uint8_t* data, size_t len)
	{
		if (len < sizeof(uint16_t))
		{
			return;
		}

		// Decrypt if crypto is enabled
		uint8_t decrypted[4096];
		const uint8_t* payload = data;
		size_t payloadLen = len;

		if (m_CryptoEnabled && len > kCryptoMACSize)
		{
			uint64_t& recvCounter = m_RecvCounters[peerIndex];
			recvCounter++;

			size_t decLen = 0;
			if (CryptoDecrypt(data, len, decrypted, decLen, m_SessionKey, recvCounter))
			{
				payload = decrypted;
				payloadLen = decLen;
			}
			else
			{
				CH_CORE_WARN("Network: Decryption failed for packet from peer {}.", peerIndex);
				return;
			}
		}

		// Parse type
		uint16_t type = 0;
		std::memcpy(&type, payload, sizeof(type));
		payload += sizeof(type);
		payloadLen -= sizeof(type);

		if (type >= MessageType_Count)
		{
			return;
		}

		if (m_PacketCallback)
		{
			m_PacketCallback(peerIndex, static_cast<MessageType>(type), payload, payloadLen);
		}
	}

	// ── Sending ──────────────────────────────────────────────────────────

	void Network::SendRaw(int peerIndex, MessageType type, const void* payload, size_t payloadLen, bool reliable)
	{
		if (!m_Host)
		{
			return;
		}

		// Build packet: [type:u16][payload]
		size_t totalLen = sizeof(type) + payloadLen;
		std::vector<uint8_t> packet(totalLen);
		std::memcpy(packet.data(), &type, sizeof(type));
		if (payloadLen > 0)
		{
			std::memcpy(packet.data() + sizeof(type), payload, payloadLen);
		}

		// Encrypt
		uint8_t encrypted[4096];
		const uint8_t* sendBuf = packet.data();
		size_t sendLen = totalLen;

		if (m_CryptoEnabled)
		{
			uint64_t& sendCounter = m_SendCounters[peerIndex];
			sendCounter++;

			size_t encLen = 0;
			if (CryptoEncrypt(packet.data(), totalLen, encrypted, encLen, m_SessionKey, sendCounter))
			{
				sendBuf = encrypted;
				sendLen = encLen;
			}
			else
			{
				CH_CORE_WARN("Network: Encryption failed for packet to peer {}.", peerIndex);
				return;
			}
		}

		ENetPacket* enetPacket = enet_packet_create(sendBuf, sendLen, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);

		if (peerIndex == kInvalidPeerHandle)
		{
			if (m_Role == Role::Host)
			{
				for (auto& [idx, peer] : m_PeerMap)
				{
					enet_peer_send(peer, reliable ? kChannel_Reliable : kChannel_Unreliable, enetPacket);
				}
			}
			else if (m_Role == Role::Client && m_ServerPeer)
			{
				enet_peer_send(m_ServerPeer, reliable ? kChannel_Reliable : kChannel_Unreliable, enetPacket);
			}
		}
		else
		{
			auto it = m_PeerMap.find(peerIndex);
			if (it != m_PeerMap.end())
			{
				enet_peer_send(it->second, reliable ? kChannel_Reliable : kChannel_Unreliable, enetPacket);
			}
		}

		enet_host_flush(m_Host);
	}

	void Network::SendPacket(int clientIndex, MessageType type, const void* data, size_t len, bool reliable)
	{
		SendRaw(clientIndex, type, data, len, reliable);
	}

	void Network::BroadcastPacket(MessageType type, bool reliable, const std::function<void(ByteWriter&)>& populate)
	{
		if (!m_Host || m_Role != Role::Host)
		{
			return;
		}

		for (auto& [clientIndex, peer] : m_PeerMap)
		{
			ByteWriter w;
			if (populate)
			{
				populate(w);
			}
			SendRaw(clientIndex, type, w.Data().data(), w.Data().size(), reliable);
		}
	}

	void Network::SendToServer(MessageType type, const void* data, size_t len, bool reliable)
	{
		if (m_Role == Role::Client && m_ServerPeer)
		{
			SendRaw(kInvalidPeerHandle, type, data, len, reliable);
		}
	}

	void Network::BroadcastSceneChange(const char* scenePath)
	{
		if (!m_Host || m_Role != Role::Host)
		{
			return;
		}

		BroadcastPacket(MessageType_SceneChange, true, [scenePath](ByteWriter& bw) {
			SceneChangeMessage msg;
			std::strncpy(msg.ScenePath, scenePath, sizeof(msg.ScenePath) - 1);
			msg.ScenePath[sizeof(msg.ScenePath) - 1] = '\0';
			msg.Encode(bw);
		});
		CH_CORE_INFO("Network: Broadcast scene change -> {}", scenePath);
	}

	// ── Packet callback ──────────────────────────────────────────────────

	void Network::SetPacketCallback(PacketCallback callback)
	{
		m_PacketCallback = std::move(callback);
	}

	void Network::ClearPacketCallback()
	{
		m_PacketCallback = nullptr;
	}

	// ── Address / connection info ────────────────────────────────────────

	std::string Network::GetListenAddress()
	{
		if (!m_Host || m_Role != Role::Host)
		{
			return {};
		}

		return "127.0.0.1:" + std::to_string(m_Port);
	}

	std::string Network::GetPublicAddress()
	{
		if (!m_PublicAddressFetched.load(std::memory_order_acquire))
		{
			return "Fetching...";
		}
		return m_CachedPublicAddress;
	}

	// ── Player list management ───────────────────────────────────────────

	void Network::SetLocalPlayerInfo(const char* name, uint8_t skinIndex)
	{
		m_LocalPlayerName = name ? name : "";
		m_LocalSkinIndex = skinIndex;

		if (m_Role != Role::Host)
		{
			return;
		}

		auto self = std::find_if(m_PlayerList.begin(), m_PlayerList.end(),
								 [](const PlayerNetInfo& p) { return p.NetworkID == HostNetworkID; });

		if (self != m_PlayerList.end())
		{
			self->Name = m_LocalPlayerName.empty() ? "Host" : m_LocalPlayerName;
			self->SkinIndex = skinIndex;
			BroadcastPlayerList();
		}
	}

	uint64_t Network::GetNetworkIDForConnection(int clientIndex) const
	{
		auto it = m_ClientIndexToNetworkID.find(clientIndex);
		return it != m_ClientIndexToNetworkID.end() ? it->second : 0;
	}

	size_t Network::GetClientCount() const
	{
		return m_PeerMap.size();
	}

	std::vector<int> Network::GetClients() const
	{
		std::vector<int> clients;
		for (auto& [idx, peer] : m_PeerMap)
		{
			clients.push_back(idx);
		}
		return clients;
	}

	bool Network::IsClientConnected(int clientIndex) const
	{
		return m_PeerMap.find(clientIndex) != m_PeerMap.end();
	}

	// ── Player info ──────────────────────────────────────────────────────

	void Network::SendPlayerInfoToHost(const char* name, uint8_t skinIndex)
	{
		if (m_Role != Role::Client || !m_ServerPeer)
		{
			return;
		}

		PlayerInfoMessage msg;
		std::strncpy(msg.Name, name ? name : "Player", sizeof(msg.Name) - 1);
		msg.Name[sizeof(msg.Name) - 1] = '\0';
		msg.SkinIndex = skinIndex;

		ByteWriter w;
		msg.Encode(w);
		SendToServer(MessageType_PlayerInfo, w.Data().data(), w.Data().size(), true);
		CH_CORE_INFO("Network: Sent player info to host (name='{}', skin={}).", msg.Name, (int)skinIndex);
	}

	void Network::BroadcastPlayerList()
	{
		if (m_Role != Role::Host || !m_Host)
		{
			return;
		}

		auto players = m_PlayerList;
		uint8_t count = static_cast<uint8_t>(std::min(players.size(), size_t(64)));

		BroadcastPacket(MessageType_PlayerList, true, [&players, count](ByteWriter& bw) {
			PlayerListMessage msg;
			msg.Count = count;
			for (int i = 0; i < count && i < 64; ++i)
			{
				msg.Entries[i].NetworkID = players[i].NetworkID;
				std::strncpy(msg.Entries[i].Name, players[i].Name.c_str(), sizeof(msg.Entries[i].Name) - 1);
				msg.Entries[i].Name[sizeof(msg.Entries[i].Name) - 1] = '\0';
				msg.Entries[i].SkinIndex = players[i].SkinIndex;
				msg.Entries[i].IsHost = players[i].IsHost;
			}
			msg.Encode(bw);
		});
		CH_CORE_INFO("Network: Broadcast player list ({} players).", count);
	}

	// ── Chat ─────────────────────────────────────────────────────────────

	void Network::SendChatMessage(const char* message)
	{
		if (!message || !message[0])
		{
			return;
		}

		if (m_Role == Role::Host && m_Host)
		{
			uint64_t senderID = m_LocalNetworkID;
			std::string senderName = m_LocalPlayerName;
			std::string chatMessage = message;

			BroadcastPacket(MessageType_ChatMessage, true, [&](ByteWriter& bw) {
				ChatMessageMessage msg;
				msg.SenderNetworkID = senderID;
				std::strncpy(msg.SenderName, senderName.c_str(), sizeof(msg.SenderName) - 1);
				msg.SenderName[sizeof(msg.SenderName) - 1] = '\0';
				std::strncpy(msg.Message, chatMessage.c_str(), sizeof(msg.Message) - 1);
				msg.Message[sizeof(msg.Message) - 1] = '\0';
				msg.Encode(bw);
			});

			m_PendingChatMessages.push_back({senderID, senderName, chatMessage});
		}
		else if (m_Role == Role::Client && m_ServerPeer)
		{
			ChatMessageMessage msg;
			msg.SenderNetworkID = m_LocalNetworkID;
			std::strncpy(msg.SenderName, m_LocalPlayerName.c_str(), sizeof(msg.SenderName) - 1);
			msg.SenderName[sizeof(msg.SenderName) - 1] = '\0';
			std::strncpy(msg.Message, message, sizeof(msg.Message) - 1);
			msg.Message[sizeof(msg.Message) - 1] = '\0';

			ByteWriter w;
			msg.Encode(w);
			SendToServer(MessageType_ChatMessage, w.Data().data(), w.Data().size(), true);
		}
	}

	void Network::StorePendingChatMessage(const ChatMessagePacket& pkt)
	{
		m_PendingChatMessages.push_back(pkt);
	}

	// ── Connection callbacks ─────────────────────────────────────────────

	void Network::OnClientConnected(int clientIndex)
	{
		if (m_ClientIndexToNetworkID.find(clientIndex) != m_ClientIndexToNetworkID.end())
		{
			return;
		}

		const uint64_t networkID = m_NextNetworkID++;
		m_ClientIndexToNetworkID[clientIndex] = networkID;

		PlayerNetInfo newPlayer;
		newPlayer.NetworkID = networkID;
		newPlayer.Name = "Player...";
		newPlayer.SkinIndex = 0;
		newPlayer.IsHost = 0;
		newPlayer.Ping = 0;
		m_PlayerList.push_back(newPlayer);

		// Send PlayerAssign to the new client
		PlayerAssignMessage assignMsg;
		assignMsg.NetworkID = networkID;
		ByteWriter w;
		assignMsg.Encode(w);
		SendRaw(clientIndex, MessageType_PlayerAssign, w.Data().data(), w.Data().size(), true);

		BroadcastPlayerList();

		CH_CORE_INFO("Network: Client connected (index={}, netID={}).", clientIndex, networkID);
	}

	void Network::OnClientDisconnected(int clientIndex)
	{
		auto idIt = m_ClientIndexToNetworkID.find(clientIndex);
		if (idIt != m_ClientIndexToNetworkID.end())
		{
			const uint64_t networkID = idIt->second;
			m_ClientIndexToNetworkID.erase(idIt);

			auto entry = std::find_if(m_PlayerList.begin(), m_PlayerList.end(),
									  [networkID](const PlayerNetInfo& p) { return p.NetworkID == networkID; });

			if (entry != m_PlayerList.end())
			{
				CH_CORE_INFO("Network: Player '{}' disconnected.", entry->Name);
				m_PlayerList.erase(entry);
			}

			CH_CORE_INFO("Network: Client disconnected (index={}).", clientIndex);
			BroadcastPlayerList();
		}
	}

} // namespace Chained
