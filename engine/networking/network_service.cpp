#include "network_service.h"
#include "engine/core/log.h"

#include <algorithm>
#include <cstring>
#include <atomic>
#include <mutex>
#include <thread>

#if CH_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace Chained
{
	/// Process-wide tracking: every Network instance registers here so the static GNS
	/// callback can route connection events to the correct owner (host or client).
	std::vector<Network*> Network::s_Instances;

	/// GNS is a process-wide singleton. Multiple Network instances (e.g. host + client
	/// in listen-server or split-screen) share one library init/kill cycle.
	int Network::s_LibRefCount = 0;

	/// Safety net — if someone forgets to call Shutdown(), the destructor cleans up.
	/// Logs a warning because explicit shutdown is preferred for deterministic ordering.
	Network::~Network()
	{
		if (m_Role != Role::Offline)
		{
			CH_CORE_WARN("Network: Destructor called before explicit Shutdown().");
			Shutdown();
		}
	}

	/// Initialize GameNetworkingSockets library (once per process) and obtain the
	/// networking interface. After this call the instance is registered in s_Instances
	/// and ready to host or connect.
	void Network::Initialize()
	{
		// The GNS library is a process-wide singleton and asserts on a second Init, so
		// refcount it — a host and a client can coexist in one process (tests, listen
		// server, split-screen) and each still owns its own sockets.
		if (s_LibRefCount == 0)
		{
			SteamDatagramErrMsg errMsg;
			if (!GameNetworkingSockets_Init(nullptr, errMsg))
			{
				CH_CORE_ERROR("Network: Failed to initialize GameNetworkingSockets: {}", errMsg);
				SetEnabled(false);
				return;
			}
		}
		++s_LibRefCount;

		m_Interface = SteamNetworkingSockets(); // global accessor, valid after Init
		s_Instances.push_back(this);
		CH_CORE_INFO("Network: Initialized successfully (GameNetworkingSockets).");
	}

	/// Tear down all networking state: close sockets, destroy poll group, release
	/// the GNS library if this was the last instance.
	void Network::Shutdown()
	{
		Disconnect(); // close all active connections first

		if (m_hListenSocket != k_HSteamListenSocket_Invalid)
		{
			m_Interface->CloseListenSocket(m_hListenSocket);
			m_hListenSocket = k_HSteamListenSocket_Invalid;
		}

		if (m_hPollGroup != k_HSteamNetPollGroup_Invalid)
		{
			m_Interface->DestroyPollGroup(m_hPollGroup);
			m_hPollGroup = k_HSteamNetPollGroup_Invalid;
		}

		m_Clients.clear();
		m_ServerConnection = k_HSteamNetConnection_Invalid;
		m_Role = Role::Offline;
		m_Port = 0;

		// Unregister and decrement refcount — last instance kills the library
		auto it = std::find(s_Instances.begin(), s_Instances.end(), this);
		if (it != s_Instances.end())
		{
			s_Instances.erase(it);

			m_Interface = nullptr;
			if (--s_LibRefCount == 0)
			{
				GameNetworkingSockets_Kill();
			}
		}

		CH_CORE_INFO("Network: Shutdown complete.");
	}

	/// Start listening for incoming connections on the given port.
	/// Creates a listen socket + poll group and switches role to Host.
	/// maxClients is informational only (GNS doesn't enforce a cap here).
	void Network::HostGame(uint16_t port, int maxClients)
	{
		if (!m_Interface)
		{
			CH_CORE_ERROR("Network: Interface not initialized.");
			SetEnabled(false);
			return;
		}

		SteamNetworkingIPAddr addr;
		addr.Clear();
		addr.m_port = port;

		// Register the status callback so we get notified of connecting/connected/disconnected peers
		SteamNetworkingConfigValue_t opt;
		opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
				   (void*)SteamNetConnectionStatusChangedCallback);

		m_hListenSocket = m_Interface->CreateListenSocketIP(addr, 1, &opt);
		if (m_hListenSocket == k_HSteamListenSocket_Invalid)
		{
			CH_CORE_ERROR("Network: Failed to create listen socket on port {}.", port);
			SetEnabled(false);
			return;
		}

		// Poll group batches incoming messages from all connections — host reads from this
		m_hPollGroup = m_Interface->CreatePollGroup();
		if (m_hPollGroup == k_HSteamNetPollGroup_Invalid)
		{
			CH_CORE_ERROR("Network: Failed to create poll group.");
			m_Interface->CloseListenSocket(m_hListenSocket);
			m_hListenSocket = k_HSteamListenSocket_Invalid;
			SetEnabled(false);
			return;
		}

		m_Role = Role::Host;
		m_Port = port;

		// UPnP: async discovery + port mapping so the host is reachable from the internet
		// without manual router configuration. Runs in a detached thread — failure is
		// non-fatal (LAN still works, and the host's public address label falls back).
		m_CachedPublicAddress.clear();
		m_PublicAddressFetched = false;
		std::thread([this, port]() {
			if (m_UpnpMapper.Initialize())
			{
				m_UpnpMapper.AddMapping(port, "UDP", "Chained Engine");
				std::string publicIP = m_UpnpMapper.GetPublicIP();
				if (!publicIP.empty())
				{
					m_CachedPublicAddress = publicIP + ":" + std::to_string(port);
					m_PublicAddressFetched = true;
					CH_CORE_INFO("Network: UPnP public address = {}", m_CachedPublicAddress);
					return;
				}
			}
#if CH_PLATFORM_WINDOWS
			std::string ip;
			HINTERNET hSession = WinHttpOpen(L"ChainedDecos/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
											 WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
			if (hSession)
			{
				HINTERNET hConnect = WinHttpConnect(hSession, L"api.ipify.org", INTERNET_DEFAULT_HTTP_PORT, 0);
				if (hConnect)
				{
					HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/", nullptr, WINHTTP_NO_REFERER,
															WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
					if (hRequest)
					{
						if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0,
											   0, 0) &&
							WinHttpReceiveResponse(hRequest, nullptr))
						{
							char buf[64] = {};
							DWORD read = 0;
							WinHttpReadData(hRequest, buf, sizeof(buf) - 1, &read);
							if (read > 0)
								ip = std::string(buf, read);
						}
						WinHttpCloseHandle(hRequest);
					}
					WinHttpCloseHandle(hConnect);
				}
				WinHttpCloseHandle(hSession);
			}
			// Trim whitespace
			while (!ip.empty() && (ip.back() == '\n' || ip.back() == '\r' || ip.back() == ' '))
				ip.pop_back();
			if (!ip.empty())
			{
				m_CachedPublicAddress = ip + ":" + std::to_string(port);
				CH_CORE_INFO("Network: Public address = {}", m_CachedPublicAddress);
			}
			else
			{
				m_CachedPublicAddress = "Port: " + std::to_string(port);
				CH_CORE_WARN("Network: Could not fetch public IP. Showing port only.");
			}
#else
			m_CachedPublicAddress = "Port: " + std::to_string(port);
#endif
			m_PublicAddressFetched = true;
		}).detach();

		CH_CORE_INFO("Network: Hosting on port {} (max {} clients).", port, maxClients);

		// The host is a player too. Without an entry here it is missing from its own
		// lobby, and the IsHost flag would fall to whichever peer got ID 1 first.
		m_LocalNetworkID = HostNetworkID;
		PlayerNetInfo self;
		self.NetworkID = HostNetworkID;
		std::strncpy(self.Name, m_LocalPlayerName.empty() ? "Host" : m_LocalPlayerName.c_str(), sizeof(self.Name) - 1);
		self.Name[sizeof(self.Name) - 1] = '\0';
		self.SkinIndex = m_LocalSkinIndex;
		self.IsHost = 1;
		m_PlayerList.push_back(self);
	}

	/// Connect to a remote host at ip:port. After this, role becomes Client and
	/// the status callback will fire with Connected/ClosedByPeer/ProblemDetectedLocally.
	void Network::ConnectTo(const std::string& ip, uint16_t port)
	{
		if (!m_Interface)
		{
			CH_CORE_ERROR("Network: Interface not initialized.");
			SetEnabled(false);
			return;
		}

		SteamNetworkingIPAddr addr;
		addr.Clear();
		std::string addrStr = ip + ":" + std::to_string(port);
		if (!addr.ParseString(addrStr.c_str()))
		{
			CH_CORE_ERROR("Network: Failed to parse address '{}'.", addrStr);
			SetEnabled(false);
			return;
		}

		SteamNetworkingConfigValue_t opt;
		opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
				   (void*)SteamNetConnectionStatusChangedCallback);

		m_ServerConnection = m_Interface->ConnectByIPAddress(addr, 1, &opt);
		if (m_ServerConnection == k_HSteamNetConnection_Invalid)
		{
			CH_CORE_ERROR("Network: Failed to initiate connection to {}:{}.", ip, port);
			SetEnabled(false);
			return;
		}

		m_Role = Role::Client;
		CH_CORE_INFO("Network: Connecting to {}:{}.", ip, port);
	}

	/// Gracefully close all connections. Client closes its server connection;
	/// host closes every client connection. Does NOT tear down sockets/poll group —
	/// call Shutdown() for full cleanup.
	void Network::Disconnect()
	{
		if (m_Interface)
		{
			if (m_ServerConnection != k_HSteamNetConnection_Invalid)
			{
				m_Interface->CloseConnection(m_ServerConnection, 0, "Client disconnect", true);
				m_ServerConnection = k_HSteamNetConnection_Invalid;
			}

			for (auto conn : m_Clients)
			{
				m_Interface->CloseConnection(conn, 0, "Server shutdown", true);
			}
			m_Clients.clear();
		}

		if (m_Role == Role::Host && m_UpnpMapper.IsAvailable())
		{
			m_UpnpMapper.RemoveMapping(m_Port, "UDP");
			m_UpnpMapper.Shutdown();
		}

		m_Role = Role::Offline;
		m_PlayerList.clear();
		m_PendingChatMessages.clear();
		m_ConnToNetworkID.clear();
		m_LocalNetworkID = 0;
		m_NextNetworkID = 2;
	}

	/// Read all pending messages from the network. Host reads from its poll group
	/// (all clients), client reads from its single server connection. Each message
	/// is deserialized as [PacketType][payload] and dispatched via m_PacketCallback.
	void Network::ReceiveMessages()
	{
		if (!m_Interface)
		{
			return;
		}

		SteamNetworkingMessage_t* msgs[32]; // batch buffer — up to 32 msgs per poll
		while (true)
		{
			// Only the host owns a poll group; a client reads from its single connection.
			int n = 0;
			if (m_hPollGroup != k_HSteamNetPollGroup_Invalid)
			{
				n = m_Interface->ReceiveMessagesOnPollGroup(m_hPollGroup, msgs, 32);
			}
			else if (m_ServerConnection != k_HSteamNetConnection_Invalid)
			{
				n = m_Interface->ReceiveMessagesOnConnection(m_ServerConnection, msgs, 32);
			}

			if (n <= 0)
			{
				break; // no more messages
			}

			for (int i = 0; i < n; ++i)
			{
				SteamNetworkingMessage_t* msg = msgs[i];

				// Wire format: [PacketType (1 byte)][payload (N bytes)]
				if (msg->m_cbSize >= sizeof(PacketType) && m_PacketCallback)
				{
					PacketType type;
					std::memcpy(&type, msg->m_pData, sizeof(PacketType));

					const uint8_t* payload = static_cast<const uint8_t*>(msg->m_pData) + sizeof(PacketType);
					size_t payloadSize = msg->m_cbSize - sizeof(PacketType);

					m_PacketCallback(type, payload, payloadSize, msg->m_conn);
				}

				msg->Release(); // return buffer to GNS — must be called for every message
			}
		}
	}

	/// Per-frame tick: run GNS callbacks (drives status changes) then drain incoming
	/// messages. Must be called every frame for the network to stay responsive.
	void Network::Update(float /*dt*/)
	{
		if (!m_Interface)
		{
			return;
		}

		// Drives connection state changes — without this the status callback never fires
		// and incoming connections are never accepted.
		m_Interface->RunCallbacks();
		ReceiveMessages();
	}

	/// Internal handler for connection state transitions. Called from the static
	/// callback after routing to the correct Network instance.
	void Network::OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
	{
		switch (pInfo->m_info.m_eState)
		{
		case k_ESteamNetworkingConnectionState_None:
			break;

		case k_ESteamNetworkingConnectionState_Connecting:
			if (m_Role == Role::Host)
			{
				// Accept incoming connection — peer is in Connecting state, we must accept or reject
				SteamNetConnectionInfo_t info;
				m_Interface->GetConnectionInfo(pInfo->m_hConn, &info);

				EResult err = m_Interface->AcceptConnection(pInfo->m_hConn);
				if (err != k_EResultOK)
				{
					CH_CORE_ERROR("Network: Failed to accept connection (err={}).", (int)err);
					m_Interface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
					return;
				}

				// Assign to poll group so ReceiveMessagesOnPollGroup picks it up
				m_Interface->SetConnectionPollGroup(pInfo->m_hConn, m_hPollGroup);
				m_Clients.push_back(pInfo->m_hConn);

				// Bind an identity to the connection right away. Everything downstream
				// (player info, avatar spawn, disconnect) resolves through this map
				// rather than guessing from list position.
				const uint64_t networkID = m_NextNetworkID++;
				m_ConnToNetworkID[pInfo->m_hConn] = networkID;

				PlayerNetInfo newPlayer;
				newPlayer.NetworkID = networkID;
				std::strncpy(newPlayer.Name, "Player...", sizeof(newPlayer.Name) - 1);
				newPlayer.SkinIndex = 0;
				newPlayer.IsHost = 0;
				m_PlayerList.push_back(newPlayer);

				CH_CORE_INFO("Network: Client connected (total: {}, netID={}).", m_Clients.size(), networkID);

				// Tell the newcomer which identity is its own, then refresh everyone.
				PlayerAssignPacket assign;
				assign.NetworkID = networkID;
				uint8_t assignBuf[PlayerAssignPacket::WireSize()];
				assign.Serialize(assignBuf);
				SendPacket(pInfo->m_hConn, PacketType::PlayerAssign, assignBuf, sizeof(assignBuf), true);

				BroadcastPlayerList();
			}
			break;

		case k_ESteamNetworkingConnectionState_Connected:
			if (m_Role == Role::Client)
			{
				CH_CORE_INFO("Network: Connected to server.");

				// Set local network ID (host will assign, but we use 0 until we get the list)
				// Automatically send our player info to the host
				if (!m_LocalPlayerName.empty())
				{
					SendPlayerInfoToHost(m_LocalPlayerName.c_str(), m_LocalSkinIndex);
				}
			}
			break;

		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
			// Either side dropped — clean up the connection handle
			if (m_Role == Role::Host)
			{
				auto it = std::find(m_Clients.begin(), m_Clients.end(), pInfo->m_hConn);
				if (it != m_Clients.end())
				{
					m_Clients.erase(it);

					// Resolve the identity through the map, not the list index — the two
					// can diverge once anyone disconnects.
					auto idIt = m_ConnToNetworkID.find(pInfo->m_hConn);
					if (idIt != m_ConnToNetworkID.end())
					{
						const uint64_t networkID = idIt->second;
						m_ConnToNetworkID.erase(idIt);

						auto entry =
							std::find_if(m_PlayerList.begin(), m_PlayerList.end(),
										 [networkID](const PlayerNetInfo& p) { return p.NetworkID == networkID; });
						if (entry != m_PlayerList.end())
						{
							CH_CORE_INFO("Network: Player '{}' disconnected.", entry->Name);
							m_PlayerList.erase(entry);
						}
					}

					CH_CORE_INFO("Network: Client disconnected (total: {}).", m_Clients.size());
					BroadcastPlayerList();
				}
			}
			else if (m_Role == Role::Client && pInfo->m_hConn == m_ServerConnection)
			{
				CH_CORE_INFO("Network: Disconnected from server.");
				m_ServerConnection = k_HSteamNetConnection_Invalid;
				m_PlayerList.clear();
			}
			m_Interface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			break;

		default:
			break;
		}
	}

	/// Static GNS callback — invoked by RunCallbacks() for ANY connection in the
	/// process. We must figure out which Network instance owns this connection
	/// by checking listen socket, outgoing connection, or client list.
	void Network::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
	{
		// Several Network instances can share the process-wide library, so route the
		// callback to the one that owns this connection rather than a single global.
		for (Network* net : s_Instances)
		{
			// Check if this connection belongs to net: incoming on its listen socket,
			// or it's the client's outgoing connection, or it's in the host's client list
			const bool ownsListenSocket = pInfo->m_info.m_hListenSocket != k_HSteamListenSocket_Invalid &&
										  pInfo->m_info.m_hListenSocket == net->m_hListenSocket;
			const bool ownsOutgoing = pInfo->m_hConn == net->m_ServerConnection;
			const bool ownsClient =
				std::find(net->m_Clients.begin(), net->m_Clients.end(), pInfo->m_hConn) != net->m_Clients.end();

			if (ownsListenSocket || ownsOutgoing || ownsClient)
			{
				net->OnConnectionStatusChanged(pInfo);
				return;
			}
		}
	}

	/// Send a raw packet to a specific connection. Wire format: [PacketType (1 byte)][payload].
	/// reliable=true uses guaranteed delivery; false uses UnreliableNoNagle for low latency.
	void Network::SendPacket(HSteamNetConnection conn, PacketType type, const void* data, size_t size, bool reliable)
	{
		if (!m_Interface || conn == k_HSteamNetConnection_Invalid)
		{
			return;
		}

		size_t totalSize = sizeof(PacketType) + size;
		std::vector<uint8_t> buffer(totalSize);

		std::memcpy(buffer.data(), &type, sizeof(PacketType));
		if (size > 0 && data)
		{
			std::memcpy(buffer.data() + sizeof(PacketType), data, size);
		}

		int flags = reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_UnreliableNoNagle;
		m_Interface->SendMessageToConnection(conn, buffer.data(), totalSize, flags, nullptr);
	}

	/// Send a packet to all connected clients (host only).
	void Network::BroadcastPacket(PacketType type, const void* data, size_t size, bool reliable)
	{
		for (auto conn : m_Clients)
		{
			SendPacket(conn, type, data, size, reliable);
		}
	}

	/// Send a packet from client to server.
	void Network::SendToServer(PacketType type, const void* data, size_t size, bool reliable)
	{
		SendPacket(m_ServerConnection, type, data, size, reliable);
	}

	/// Convenience: broadcast a scene path to all clients. The scene change packet
	/// is always reliable so clients don't miss it.
	void Network::BroadcastSceneChange(const char* scenePath)
	{
		if (!scenePath || !scenePath[0])
		{
			return;
		}

		SceneChangePacket pkt;
		std::strncpy(pkt.ScenePath, scenePath, sizeof(pkt.ScenePath) - 1);
		pkt.ScenePath[sizeof(pkt.ScenePath) - 1] = '\0'; // ensure null-termination

		uint8_t buffer[SceneChangePacket::WireSize()];
		pkt.Serialize(buffer);
		BroadcastPacket(PacketType::SceneChange, buffer, sizeof(buffer), true);

		CH_CORE_INFO("Network: Broadcast scene change -> {}", scenePath);
	}

	// ---- Packet Callback ----

	void Network::SetPacketCallback(PacketCallback callback)
	{
		m_PacketCallback = std::move(callback);
	}

	void Network::ClearPacketCallback()
	{
		m_PacketCallback = nullptr;
	}

	// ---- Listen address ----

	/// Get the LAN address of the listen socket (e.g. "192.168.1.5:7777").
	std::string Network::GetListenAddress()
	{
		if (!m_Interface || m_hListenSocket == k_HSteamListenSocket_Invalid)
		{
			return {};
		}

		SteamNetworkingIPAddr addr;
		if (m_Interface->GetListenSocketAddress(m_hListenSocket, &addr))
		{
			char buf[SteamNetworkingIPAddr::k_cchMaxString];
			addr.ToString(buf, sizeof(buf), true);
			return std::string(buf);
		}
		return {};
	}

	/// Returns the public IP:PORT string (e.g. "203.0.113.5:7777") suitable
	/// for sharing with friends over the internet.
	/// The first call spawns an async WinHTTP fetch; subsequent calls are instant.
	std::string Network::GetPublicAddress()
	{
		if (!m_PublicAddressFetched)
		{
			return "Fetching...";
		}
		return m_CachedPublicAddress;
	}

	// ---- Player list ----

	/// Store the local player's name and skin. Called before connecting or hosting.
	/// When already hosting, the host's own list entry is updated in place so the
	/// lobby reflects a nickname or skin chosen after HostGame().
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
			std::strncpy(self->Name, m_LocalPlayerName.empty() ? "Host" : m_LocalPlayerName.c_str(),
						 sizeof(self->Name) - 1);
			self->Name[sizeof(self->Name) - 1] = '\0';
			self->SkinIndex = skinIndex;
			BroadcastPlayerList();
		}
	}

	/// Identity assigned to a peer when its connection was accepted.
	uint64_t Network::GetNetworkIDForConnection(HSteamNetConnection conn) const
	{
		auto it = m_ConnToNetworkID.find(conn);
		return it != m_ConnToNetworkID.end() ? it->second : 0;
	}

	/// Client sends its player info to the host after connecting.
	void Network::SendPlayerInfoToHost(const char* name, uint8_t skinIndex)
	{
		if (m_Role != Role::Client)
		{
			return;
		}

		PlayerInfoPacket pkt;
		std::strncpy(pkt.Name, name ? name : "Player", sizeof(pkt.Name) - 1);
		pkt.Name[sizeof(pkt.Name) - 1] = '\0';
		pkt.SkinIndex = skinIndex;

		uint8_t buffer[PlayerInfoPacket::WireSize()];
		pkt.Serialize(buffer);
		SendToServer(PacketType::PlayerInfo, buffer, sizeof(buffer), true);

		CH_CORE_INFO("Network: Sent player info to host (name='{}', skin={}).", pkt.Name, (int)skinIndex);
	}

	/// Host broadcasts the full player list to all clients.
	void Network::BroadcastPlayerList()
	{
		if (m_Role != Role::Host)
		{
			return;
		}

		constexpr size_t MaxEntries = 32;
		size_t count = std::min(m_PlayerList.size(), MaxEntries);
		size_t entrySize = sizeof(PlayerListEntry);
		size_t totalSize = PlayerListPacket::HeaderSize() + count * entrySize;

		std::vector<uint8_t> buffer(totalSize);

		// Header: count
		uint8_t staticCount = static_cast<uint8_t>(count);
		std::memcpy(buffer.data(), &staticCount, PlayerListPacket::HeaderSize());

		// Entries
		size_t offset = PlayerListPacket::HeaderSize();
		for (size_t i = 0; i < count; ++i)
		{
			PlayerListEntry entry;
			entry.NetworkID = m_PlayerList[i].NetworkID;
			std::strncpy(entry.Name, m_PlayerList[i].Name, sizeof(entry.Name) - 1);
			entry.Name[sizeof(entry.Name) - 1] = '\0';
			entry.SkinIndex = m_PlayerList[i].SkinIndex;
			entry.IsHost = m_PlayerList[i].IsHost;

			std::memcpy(buffer.data() + offset, &entry, entrySize);
			offset += entrySize;
		}

		BroadcastPacket(PacketType::PlayerList, buffer.data(), totalSize, true);
		CH_CORE_INFO("Network: Broadcast player list ({} players).", count);
	}

	/// Store a chat message in the pending queue for C# consumption.
	void Network::StorePendingChatMessage(const ChatMessagePacket& pkt)
	{
		m_PendingChatMessages.push_back(pkt);
	}

	/// Send a chat message to all peers (host broadcasts, client sends to server).
	void Network::SendChatMessage(const char* message)
	{
		if (!message || !message[0])
		{
			return;
		}

		ChatMessagePacket pkt;
		pkt.SenderNetworkID = m_LocalNetworkID;
		std::strncpy(pkt.SenderName, m_LocalPlayerName.c_str(), sizeof(pkt.SenderName) - 1);
		pkt.SenderName[sizeof(pkt.SenderName) - 1] = '\0';
		std::strncpy(pkt.Message, message, sizeof(pkt.Message) - 1);
		pkt.Message[sizeof(pkt.Message) - 1] = '\0';

		uint8_t buffer[ChatMessagePacket::WireSize()];
		pkt.Serialize(buffer);

		if (m_Role == Role::Host)
		{
			// Host broadcasts to all clients (and echoes to self)
			BroadcastPacket(PacketType::ChatMessage, buffer, sizeof(buffer), true);
			// Also store for host's own UI
			m_PendingChatMessages.push_back(pkt);
		}
		else if (m_Role == Role::Client)
		{
			SendToServer(PacketType::ChatMessage, buffer, sizeof(buffer), true);
		}
	}

} // namespace Chained
