#include "network_service.h"
#include "firewall_helper.h"
#include <thread>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

namespace Chained
{
	static std::string FetchPublicIPFromWeb()
	{
		struct addrinfo hints = {}, *res = nullptr;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo("api.ipify.org", "80", &hints, &res) != 0 || !res)
		{
			return {};
		}

		auto sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sock == -1
#ifdef _WIN32
			|| sock == INVALID_SOCKET
#endif
		)
		{
			freeaddrinfo(res);
			return {};
		}

#ifdef _WIN32
		DWORD timeout = 3000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
		struct timeval tv = {3, 0};
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

		if (connect(sock, res->ai_addr, (int)res->ai_addrlen) != 0)
		{
#ifdef _WIN32
			closesocket(sock);
#else
			close(sock);
#endif
			freeaddrinfo(res);
			return {};
		}
		freeaddrinfo(res);

		const char* request =
			"GET / HTTP/1.1\r\nHost: api.ipify.org\r\nConnection: close\r\nUser-Agent: ChainedEngine\r\n\r\n";
		send(sock, request, (int)strlen(request), 0);

		char buffer[1024] = {};
		int totalBytes = 0;
		int bytes = 0;
		while ((bytes = recv(sock, buffer + totalBytes, sizeof(buffer) - 1 - totalBytes, 0)) > 0)
		{
			totalBytes += bytes;
		}

#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif

		if (totalBytes <= 0)
		{
			return {};
		}
		buffer[totalBytes] = '\0';

		const char* body = strstr(buffer, "\r\n\r\n");
		if (!body)
		{
			return {};
		}
		body += 4;

		std::string ip(body);
		ip.erase(std::remove_if(ip.begin(), ip.end(), [](unsigned char c) { return std::isspace(c); }), ip.end());
		return ip;
	}
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

		// Remove Windows Firewall rule
		if (m_FirewallRuleActive && portToUnmap != 0)
		{
			Firewall::RemoveUDPRule(portToUnmap);
			m_FirewallRuleActive = false;
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

		if (!m_TestMode)
		{
			if (!m_UpnpMapper.IsAvailable())
			{
				m_UpnpMapper.Initialize();
			}
			if (m_UpnpMapper.IsAvailable())
			{
				m_UpnpMapper.AddMapping(port, "UDP", "ChainedDecos");
				CH_CORE_INFO("Network: UPnP mapping added.");
			}
			else
			{
				CH_CORE_WARN("Network: UPnP unavailable — players must forward port {} manually.", port);
			}

			// Add Windows Firewall inbound rule
			m_FirewallRuleActive = Firewall::AddUDPRule(port);
			if (!m_FirewallRuleActive)
			{
				CH_CORE_WARN("Network: Could not add firewall rule. Run as admin to allow inbound connections.");
			}
		}

		{
			std::lock_guard<std::mutex> lock(m_PublicIPMutex);
			m_CachedPublicIP = "Fetching...";
		}

		// Asynchronously resolve true public WAN IP from web (api.ipify.org)
		std::thread([this, port]() {
			std::string ip = FetchPublicIPFromWeb();
			std::lock_guard<std::mutex> lock(m_PublicIPMutex);
			if (!ip.empty())
			{
				m_CachedPublicIP = ip + ":" + std::to_string(port);
				CH_CORE_INFO("Network: Public IP resolved: {}", m_CachedPublicIP);
			}
			else
			{
				// Fallback to UPnP WAN IP if available
				std::string upnpIp = m_UpnpMapper.GetPublicIP();
				if (!upnpIp.empty())
				{
					m_CachedPublicIP = upnpIp + ":" + std::to_string(port);
				}
				else
				{
					m_CachedPublicIP = "Manual (Port " + std::to_string(port) + ")";
				}
			}
		}).detach();

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
		{
			std::lock_guard<std::mutex> lock(m_PublicIPMutex);
			m_CachedPublicIP.clear();
		}
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
		if (m_UpnpMapper.IsAvailable() && m_UpnpMapper.GetLanIP()[0] != '\0')
		{
			return std::string(m_UpnpMapper.GetLanIP()) + ":" + std::to_string(m_Session.GetPort());
		}
		return m_Session.GetListenAddress();
	}

	std::string Network::GetPublicAddress()
	{
		std::lock_guard<std::mutex> lock(m_PublicIPMutex);
		if (!m_CachedPublicIP.empty())
		{
			return m_CachedPublicIP;
		}
		return "Fetching...";
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
