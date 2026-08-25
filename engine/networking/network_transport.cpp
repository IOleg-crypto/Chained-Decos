#include "network_transport.h"
#include "network_session.h"
#include "net_packet.h"
#include <sodium.h>
#include <cstring>
#include <random>

namespace Chained
{
	static constexpr size_t kCryptoNonceSize = 24;
	static constexpr size_t kCryptoMACSize = 16;

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

	void NetworkTransport::SetPacketCallback(PacketCallback callback)
	{
		m_PacketCallback = std::move(callback);
	}
	void NetworkTransport::ClearPacketCallback()
	{
		m_PacketCallback = nullptr;
	}
	void NetworkTransport::SetSessionKey(const uint8_t key[32])
	{
		std::memcpy(m_SessionKey, key, 32);
	}
	void NetworkTransport::ResetCounters()
	{
		m_SendCounters.clear();
		m_RecvCounters.clear();
	}

	void NetworkTransport::SendRaw(int peerIndex, MessageType type, const void* payload, size_t payloadLen,
								   bool reliable)
	{
		ENetHost* host = m_Session ? m_Session->GetHost() : nullptr;
		if (!host)
		{
			return;
		}

		size_t totalLen = sizeof(type) + payloadLen;
		std::vector<uint8_t> packet(totalLen);
		std::memcpy(packet.data(), &type, sizeof(type));
		if (payloadLen > 0)
		{
			std::memcpy(packet.data() + sizeof(type), payload, payloadLen);
		}

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
				CH_CORE_WARN("NetworkTransport: Encryption failed for peer {}.", peerIndex);
				return;
			}
		}

		ENetPacket* enetPacket = enet_packet_create(sendBuf, sendLen, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
		int channel = reliable ? kChannel_Reliable : kChannel_Unreliable;

		if (peerIndex == kInvalidPeerHandle)
		{
			if (m_Session->IsHost())
			{
				for (auto& [idx, peer] : m_Session->GetPeerMap())
				{
					enet_peer_send(peer, channel, enetPacket);
				}
			}
			else if (m_Session->IsClient() && m_Session->GetServerPeer())
			{
				enet_peer_send(m_Session->GetServerPeer(), channel, enetPacket);
			}
		}
		else
		{
			ENetPeer* peer = m_Session->GetPeerForClient(peerIndex);
			if (peer)
			{
				enet_peer_send(peer, channel, enetPacket);
			}
		}

		enet_host_flush(host);
	}

	void NetworkTransport::HandleIncomingPacket(int peerIndex, const uint8_t* data, size_t len)
	{
		if (len < sizeof(uint16_t))
		{
			return;
		}

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
				CH_CORE_WARN("NetworkTransport: Decryption failed for peer {}.", peerIndex);
				return;
			}
		}

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

	void NetworkTransport::SendPacket(int peerIndex, MessageType type, const void* data, size_t len, bool reliable)
	{
		SendRaw(peerIndex, type, data, len, reliable);
	}

	void NetworkTransport::BroadcastPacket(MessageType type, bool reliable,
										   const std::function<void(ByteWriter&)>& populate)
	{
		if (!m_Session || !m_Session->IsHost())
		{
			return;
		}
		const auto& peerMap = m_Session->GetPeerMap();
		for (auto& [clientIndex, peer] : peerMap)
		{
			ByteWriter w;
			if (populate)
			{
				populate(w);
			}
			SendRaw(clientIndex, type, w.Data().data(), w.Data().size(), reliable);
		}
	}

	void NetworkTransport::SendToServer(MessageType type, const void* data, size_t len, bool reliable)
	{
		if (!m_Session || !m_Session->IsClient() || !m_Session->GetServerPeer())
		{
			return;
		}
		SendRaw(kInvalidPeerHandle, type, data, len, reliable);
	}

	void NetworkTransport::BroadcastSceneChange(const char* scenePath)
	{
		if (!m_Session || !m_Session->IsHost())
		{
			return;
		}
		BroadcastPacket(MessageType_SceneChange, true, [scenePath](ByteWriter& bw) {
			SceneChangeMessage msg;
			std::strncpy(msg.ScenePath, scenePath, sizeof(msg.ScenePath) - 1);
			msg.ScenePath[sizeof(msg.ScenePath) - 1] = '\0';
			msg.Encode(bw);
		});
		CH_CORE_INFO("NetworkTransport: Broadcast scene change -> {}", scenePath);
	}

	void NetworkTransport::SendPlayerInfoToHost(const char* name, uint8_t skinIndex, uint64_t /*localNetworkID*/)
	{
		if (!m_Session || m_Session->GetRole() != Role::Client || !m_Session->GetServerPeer())
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
		CH_CORE_INFO("NetworkTransport: Sent player info (name='{}', skin={}).", msg.Name, (int)skinIndex);
	}

	void NetworkTransport::BroadcastPlayerList(const std::vector<PlayerNetInfo>& playerList)
	{
		if (!m_Session || !m_Session->IsHost())
		{
			return;
		}
		uint8_t count = static_cast<uint8_t>(std::min(playerList.size(), size_t(64)));
		BroadcastPacket(MessageType_PlayerList, true, [&playerList, count](ByteWriter& bw) {
			PlayerListMessage msg;
			msg.Count = count;
			for (int i = 0; i < count && i < 64; ++i)
			{
				msg.Entries[i].NetworkID = playerList[i].NetworkID;
				std::strncpy(msg.Entries[i].Name, playerList[i].Name.c_str(), sizeof(msg.Entries[i].Name) - 1);
				msg.Entries[i].Name[sizeof(msg.Entries[i].Name) - 1] = '\0';
				msg.Entries[i].SkinIndex = playerList[i].SkinIndex;
				msg.Entries[i].IsHost = playerList[i].IsHost;
			}
			msg.Encode(bw);
		});
		CH_CORE_INFO("NetworkTransport: Broadcast player list ({} players).", count);
	}

	void NetworkTransport::SendChatMessage(const char* message, uint64_t localNetworkID,
										   const std::string& localPlayerName)
	{
		if (!message || !message[0] || !m_Session)
		{
			return;
		}

		if (m_Session->IsHost())
		{
			BroadcastPacket(MessageType_ChatMessage, true, [&](ByteWriter& bw) {
				ChatMessageMessage msg;
				msg.SenderNetworkID = localNetworkID;
				std::strncpy(msg.SenderName, localPlayerName.c_str(), sizeof(msg.SenderName) - 1);
				msg.SenderName[sizeof(msg.SenderName) - 1] = '\0';
				std::strncpy(msg.Message, message, sizeof(msg.Message) - 1);
				msg.Message[sizeof(msg.Message) - 1] = '\0';
				msg.Encode(bw);
			});
			ChatMessagePacket pkt;
			pkt.SenderNetworkID = localNetworkID;
			pkt.SenderName = localPlayerName;
			pkt.Message = message;
			m_PendingChatMessages.push_back(pkt);
		}
		else if (m_Session->IsClient() && m_Session->GetServerPeer())
		{
			ChatMessageMessage msg;
			msg.SenderNetworkID = localNetworkID;
			std::strncpy(msg.SenderName, localPlayerName.c_str(), sizeof(msg.SenderName) - 1);
			msg.SenderName[sizeof(msg.SenderName) - 1] = '\0';
			std::strncpy(msg.Message, message, sizeof(msg.Message) - 1);
			msg.Message[sizeof(msg.Message) - 1] = '\0';
			ByteWriter w;
			msg.Encode(w);
			SendToServer(MessageType_ChatMessage, w.Data().data(), w.Data().size(), true);
		}
	}
} // namespace Chained
