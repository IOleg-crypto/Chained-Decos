#ifndef CH_NETWORK_TRANSPORT_H
#define CH_NETWORK_TRANSPORT_H

#include "network_types.h"
#include "net_packet.h"

#include <cstdint>
#include <functional>
#include <vector>
#include <unordered_map>

struct _ENetHost;
struct _ENetPeer;
typedef struct _ENetHost ENetHost;
typedef struct _ENetPeer ENetPeer;

namespace Chained
{

	class NetworkSession;

	class NetworkTransport
	{
	public:
		using PacketCallback = std::function<void(int peerIndex, MessageType type, const uint8_t* data, size_t len)>;

		NetworkTransport() = default;
		~NetworkTransport() = default;

		NetworkTransport(const NetworkTransport&) = delete;
		NetworkTransport& operator=(const NetworkTransport&) = delete;

		void SetSession(NetworkSession* session)
		{
			m_Session = session;
		}

		void SetPacketCallback(PacketCallback callback);
		void ClearPacketCallback();

		void SendPacket(int peerIndex, MessageType type, const void* data, size_t len, bool reliable = true);
		void BroadcastPacket(MessageType type, bool reliable, const std::function<void(ByteWriter&)>& populate);
		void SendToServer(MessageType type, const void* data, size_t len, bool reliable = true);
		void BroadcastSceneChange(const char* scenePath);

		void SendPlayerInfoToHost(const char* name, uint8_t skinIndex, uint64_t localNetworkID);
		void BroadcastPlayerList(const std::vector<struct PlayerNetInfo>& playerList);
		void SendChatMessage(const char* message, uint64_t localNetworkID, const std::string& localPlayerName);

		void HandleIncomingPacket(int peerIndex, const uint8_t* data, size_t len);

		void SetCryptoEnabled(bool enabled)
		{
			m_CryptoEnabled = enabled;
		}
		bool IsCryptoEnabled() const
		{
			return m_CryptoEnabled;
		}

		void ResetCounters();
		void SetSessionKey(const uint8_t key[32]);

		void ClearPendingChatMessages()
		{
			m_PendingChatMessages.clear();
		}
		bool HasPendingChatMessages() const
		{
			return !m_PendingChatMessages.empty();
		}
		const std::vector<ChatMessagePacket>& GetPendingChatMessages() const
		{
			return m_PendingChatMessages;
		}
		void StorePendingChatMessage(const ChatMessagePacket& pkt)
		{
			m_PendingChatMessages.push_back(pkt);
		}

	private:
		void SendRaw(int peerIndex, MessageType type, const void* payload, size_t payloadLen, bool reliable);

		NetworkSession* m_Session = nullptr;
		PacketCallback m_PacketCallback;
		std::vector<ChatMessagePacket> m_PendingChatMessages;

		uint8_t m_SessionKey[32] = {};
		bool m_CryptoEnabled = false;
		std::unordered_map<int, uint64_t> m_SendCounters;
		std::unordered_map<int, uint64_t> m_RecvCounters;
	};

} // namespace Chained

#endif // CH_NETWORK_TRANSPORT_H
