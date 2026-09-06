#ifndef CH_JUICE_ICE_DRIVER_H
#define CH_JUICE_ICE_DRIVER_H

#include "network_driver.h"
#include "network_types.h"

#include <juice/juice.h>

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Chained
{
	/// WebRTC ICE driver powered by libjuice.
	/// Resolves NAT traversal via STUN for same-network and port-forwarded connections.
	class JuiceIceDriver : public INetworkDriver
	{
	public:
		JuiceIceDriver();
		~JuiceIceDriver() override;

		NetworkError Initialize() override;
		void Shutdown() override;

		NetworkError Host(uint16_t port, int maxClients) override;
		NetworkError Connect(const std::string& target, uint16_t port) override;
		void Disconnect() override;
		void DisconnectPeer(int peerIndex) override;

		void SendPacket(int peerIndex, ePacketChannel channel, const void* data, size_t len, bool reliable) override;
		void BroadcastPacket(ePacketChannel channel, const void* data, size_t len, bool reliable) override;

		void PollEvents(std::vector<NetworkDriverEvent>& outEvents) override;

		Role GetRole() const override
		{
			return m_Role.load(std::memory_order_relaxed);
		}
		bool IsConnected() const override
		{
			return m_Role.load(std::memory_order_relaxed) != Role::Offline;
		}
		bool IsFullyConnected() const override
		{
			return m_FullyConnected.load(std::memory_order_relaxed);
		}
		uint16_t GetPort() const override
		{
			return m_Port.load(std::memory_order_relaxed);
		}
		int GetMaxClients() const override
		{
			return m_MaxClients.load(std::memory_order_relaxed);
		}
		std::string GetListenAddress() const override;
		uint32_t GetPeerRtt(int peerIndex) const override;
		bool IsPeerConnected(int peerIndex) const override;

		// ICE-specific description export/import for signaling
		std::string GetLocalSessionToken() const;
		bool SetRemoteSessionToken(const std::string& token);

	private:
		static void OnStateChanged(juice_agent_t* agent, juice_state_t state, void* user_ptr);
		static void OnCandidate(juice_agent_t* agent, const char* sdp, void* user_ptr);
		static void OnGatheringDone(juice_agent_t* agent, void* user_ptr);
		static void OnRecv(juice_agent_t* agent, const char* data, size_t size, void* user_ptr);

		void HandleStateChanged(juice_agent_t* agent, juice_state_t state);
		void HandleCandidate(juice_agent_t* agent, const std::string& sdp);
		void HandleGatheringDone(juice_agent_t* agent);
		void HandleRecv(juice_agent_t* agent, const uint8_t* data, size_t size);

	private:
		std::atomic<bool> m_Initialized{false};
		std::atomic<Role> m_Role{Role::Offline};
		std::atomic<bool> m_FullyConnected{false};
		std::atomic<uint16_t> m_Port{0};
		std::atomic<int> m_MaxClients{1};

		mutable std::mutex m_AgentMutex;
		juice_agent_t* m_Agent = nullptr;

		mutable std::mutex m_InboundMutex;
		std::vector<NetworkDriverEvent> m_InboundQueue;

		mutable std::mutex m_SessionTokenMutex;
		std::string m_LocalSessionToken;
		std::vector<std::string> m_LocalCandidates;
		bool m_GatheringDone = false;
	};

} // namespace Chained

#endif // CH_JUICE_ICE_DRIVER_H
