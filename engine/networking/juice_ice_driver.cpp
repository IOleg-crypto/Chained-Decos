#include "juice_ice_driver.h"
#include "engine/core/log.h"

#include <cstring>
#include <sstream>

namespace Chained
{
	static const char* s_DefaultStunHost = "stun.l.google.com";
	static constexpr uint16_t s_DefaultStunPort = 19302;

	JuiceIceDriver::JuiceIceDriver()
	{
	}

	JuiceIceDriver::~JuiceIceDriver()
	{
		Shutdown();
	}

	NetworkError JuiceIceDriver::Initialize()
	{
		m_Initialized.store(true, std::memory_order_relaxed);
		CH_CORE_INFO("[Network][ICE] Initialized libjuice ICE/STUN engine.");
		return NetworkError::None;
	}

	void JuiceIceDriver::Shutdown()
	{
		Disconnect();
		m_Initialized.store(false, std::memory_order_relaxed);
	}

	NetworkError JuiceIceDriver::Host(uint16_t port, int maxClients)
	{
		if (!m_Initialized.load(std::memory_order_relaxed))
		{
			Initialize();
		}

		Disconnect();

		juice_config_t config = {};
		config.concurrency_mode = JUICE_CONCURRENCY_MODE_THREAD;

		// STUN: Google public STUN server
		config.stun_server_host = s_DefaultStunHost;
		config.stun_server_port = s_DefaultStunPort;

		config.local_port_range_begin = port > 0 ? port : 4588;
		config.local_port_range_end = (port > 0 ? port : 4588) + 20;
		config.cb_state_changed = &JuiceIceDriver::OnStateChanged;
		config.cb_candidate = &JuiceIceDriver::OnCandidate;
		config.cb_gathering_done = &JuiceIceDriver::OnGatheringDone;
		config.cb_recv = &JuiceIceDriver::OnRecv;
		config.user_ptr = this;

		{
			std::lock_guard<std::mutex> lock(m_AgentMutex);
			m_Agent = juice_create(&config);
			if (!m_Agent)
			{
				CH_CORE_ERROR("[Network][ICE] Failed to create ICE agent.");
				return NetworkError::CreateHostFailed;
			}
		}

		m_Port.store(port, std::memory_order_relaxed);
		m_MaxClients.store(maxClients, std::memory_order_relaxed);
		m_Role.store(Role::Host, std::memory_order_relaxed);
		m_FullyConnected.store(false, std::memory_order_relaxed);

		{
			std::lock_guard<std::mutex> lock(m_SessionTokenMutex);
			m_LocalCandidates.clear();
			m_GatheringDone = false;
		}

		// Begin candidate gathering (Host + STUN srflx)
		juice_gather_candidates(m_Agent);
		CH_CORE_INFO("[Network][ICE] Host ICE agent created. Gathering candidates (STUN)...");

		return NetworkError::None;
	}

	NetworkError JuiceIceDriver::Connect(const std::string& target, uint16_t port)
	{
		if (!m_Initialized.load(std::memory_order_relaxed))
		{
			Initialize();
		}

		Disconnect();

		juice_config_t config = {};
		config.concurrency_mode = JUICE_CONCURRENCY_MODE_THREAD;

		config.stun_server_host = s_DefaultStunHost;
		config.stun_server_port = s_DefaultStunPort;

		config.cb_state_changed = &JuiceIceDriver::OnStateChanged;
		config.cb_candidate = &JuiceIceDriver::OnCandidate;
		config.cb_gathering_done = &JuiceIceDriver::OnGatheringDone;
		config.cb_recv = &JuiceIceDriver::OnRecv;
		config.user_ptr = this;

		{
			std::lock_guard<std::mutex> lock(m_AgentMutex);
			m_Agent = juice_create(&config);
			if (!m_Agent)
			{
				CH_CORE_ERROR("[Network][ICE] Failed to create client ICE agent.");
				return NetworkError::ConnectFailed;
			}
		}

		m_Port.store(port, std::memory_order_relaxed);
		m_MaxClients.store(1, std::memory_order_relaxed);
		m_Role.store(Role::Client, std::memory_order_relaxed);
		m_FullyConnected.store(false, std::memory_order_relaxed);

		juice_gather_candidates(m_Agent);
		CH_CORE_INFO("[Network][ICE] Client ICE agent connecting to target (STUN enabled)...");

		if (!target.empty())
		{
			SetRemoteSessionToken(target);
		}

		return NetworkError::None;
	}

	void JuiceIceDriver::Disconnect()
	{
		std::lock_guard<std::mutex> lock(m_AgentMutex);
		if (m_Agent)
		{
			juice_destroy(m_Agent);
			m_Agent = nullptr;
		}

		m_Role.store(Role::Offline, std::memory_order_relaxed);
		m_FullyConnected.store(false, std::memory_order_relaxed);
	}

	void JuiceIceDriver::DisconnectPeer(int /*peerIndex*/)
	{
		Disconnect();
	}

	void JuiceIceDriver::SendPacket(int /*peerIndex*/, ePacketChannel channel, const void* data, size_t len,
									bool /*reliable*/)
	{
		std::lock_guard<std::mutex> lock(m_AgentMutex);
		if (!m_Agent || !m_FullyConnected.load(std::memory_order_relaxed))
		{
			return;
		}

		// Prepend 1 byte for channel index
		std::vector<uint8_t> buffer(len + 1);
		buffer[0] = static_cast<uint8_t>(channel);
		if (len > 0 && data)
		{
			std::memcpy(buffer.data() + 1, data, len);
		}

		juice_send(m_Agent, reinterpret_cast<const char*>(buffer.data()), buffer.size());
	}

	void JuiceIceDriver::BroadcastPacket(ePacketChannel channel, const void* data, size_t len, bool reliable)
	{
		SendPacket(0, channel, data, len, reliable);
	}

	void JuiceIceDriver::PollEvents(std::vector<NetworkDriverEvent>& outEvents)
	{
		std::lock_guard<std::mutex> lock(m_InboundMutex);
		if (!m_InboundQueue.empty())
		{
			outEvents.insert(outEvents.end(), std::make_move_iterator(m_InboundQueue.begin()),
							 std::make_move_iterator(m_InboundQueue.end()));
			m_InboundQueue.clear();
		}
	}

	std::string JuiceIceDriver::GetListenAddress() const
	{
		std::lock_guard<std::mutex> lock(m_AgentMutex);
		if (!m_Agent)
		{
			return "0.0.0.0:" + std::to_string(m_Port.load(std::memory_order_relaxed));
		}
		char local[JUICE_MAX_ADDRESS_STRING_LEN] = {};
		char remote[JUICE_MAX_ADDRESS_STRING_LEN] = {};
		if (juice_get_selected_addresses(m_Agent, local, sizeof(local), remote, sizeof(remote)) == JUICE_ERR_SUCCESS)
		{
			return std::string(local);
		}
		return "0.0.0.0:" + std::to_string(m_Port.load(std::memory_order_relaxed));
	}

	uint32_t JuiceIceDriver::GetPeerRtt(int /*peerIndex*/) const
	{
		return 35; // Nominally low estimate for active ICE pair
	}

	bool JuiceIceDriver::IsPeerConnected(int /*peerIndex*/) const
	{
		return m_FullyConnected.load(std::memory_order_relaxed);
	}

	std::string JuiceIceDriver::GetLocalSessionToken() const
	{
		std::lock_guard<std::mutex> lock(m_AgentMutex);
		if (!m_Agent)
		{
			return "";
		}

		char sdp[JUICE_MAX_SDP_STRING_LEN] = {};
		if (juice_get_local_description(m_Agent, sdp, sizeof(sdp)) == JUICE_ERR_SUCCESS)
		{
			return std::string(sdp);
		}
		return "";
	}

	bool JuiceIceDriver::SetRemoteSessionToken(const std::string& token)
	{
		std::lock_guard<std::mutex> lock(m_AgentMutex);
		if (!m_Agent || token.empty())
		{
			return false;
		}

		int res = juice_set_remote_description(m_Agent, token.c_str());
		if (res == JUICE_ERR_SUCCESS)
		{
			juice_set_remote_gathering_done(m_Agent);
			CH_CORE_INFO("[Network][ICE] Remote description set successfully. Probing ICE connectivity...");
			return true;
		}
		else
		{
			CH_CORE_WARN("[Network][ICE] Failed to set remote description (err={}).", res);
			return false;
		}
	}

	// ── Callbacks ──────────────────────────────────────────────────────────

	void JuiceIceDriver::OnStateChanged(juice_agent_t* agent, juice_state_t state, void* user_ptr)
	{
		if (auto* self = static_cast<JuiceIceDriver*>(user_ptr))
		{
			self->HandleStateChanged(agent, state);
		}
	}

	void JuiceIceDriver::OnCandidate(juice_agent_t* agent, const char* sdp, void* user_ptr)
	{
		if (auto* self = static_cast<JuiceIceDriver*>(user_ptr))
		{
			self->HandleCandidate(agent, sdp ? sdp : "");
		}
	}

	void JuiceIceDriver::OnGatheringDone(juice_agent_t* agent, void* user_ptr)
	{
		if (auto* self = static_cast<JuiceIceDriver*>(user_ptr))
		{
			self->HandleGatheringDone(agent);
		}
	}

	void JuiceIceDriver::OnRecv(juice_agent_t* agent, const char* data, size_t size, void* user_ptr)
	{
		if (auto* self = static_cast<JuiceIceDriver*>(user_ptr))
		{
			self->HandleRecv(agent, reinterpret_cast<const uint8_t*>(data), size);
		}
	}

	void JuiceIceDriver::HandleStateChanged(juice_agent_t* agent, juice_state_t state)
	{
		CH_CORE_INFO("[Network][ICE] Agent state changed: {}", juice_state_to_string(state));

		if (state == JUICE_STATE_CONNECTED || state == JUICE_STATE_COMPLETED)
		{
			m_FullyConnected.store(true, std::memory_order_relaxed);

			char localAddr[64] = {};
			char remoteAddr[64] = {};
			juice_get_selected_addresses(agent, localAddr, sizeof(localAddr), remoteAddr, sizeof(remoteAddr));
			CH_CORE_INFO("[Network][ICE] Established ICE connectivity: Local={} <-> Remote={}", localAddr, remoteAddr);

			NetworkDriverEvent ev;
			ev.Type = NetworkDriverEventType::Connected;
			ev.PeerIndex = 0;

			std::lock_guard<std::mutex> lock(m_InboundMutex);
			m_InboundQueue.push_back(std::move(ev));
		}
		else if (state == JUICE_STATE_FAILED || state == JUICE_STATE_DISCONNECTED)
		{
			if (m_FullyConnected.load(std::memory_order_relaxed))
			{
				m_FullyConnected.store(false, std::memory_order_relaxed);

				NetworkDriverEvent ev;
				ev.Type = NetworkDriverEventType::Disconnected;
				ev.PeerIndex = 0;

				std::lock_guard<std::mutex> lock(m_InboundMutex);
				m_InboundQueue.push_back(std::move(ev));
			}
		}
	}

	void JuiceIceDriver::HandleCandidate(juice_agent_t* /*agent*/, const std::string& sdp)
	{
		std::lock_guard<std::mutex> lock(m_SessionTokenMutex);
		m_LocalCandidates.push_back(sdp);
		CH_CORE_TRACE("[Network][ICE] Gathered candidate: {}", sdp);
	}

	void JuiceIceDriver::HandleGatheringDone(juice_agent_t* agent)
	{
		std::lock_guard<std::mutex> lock(m_SessionTokenMutex);
		m_GatheringDone = true;

		char sdp[JUICE_MAX_SDP_STRING_LEN] = {};
		if (juice_get_local_description(agent, sdp, sizeof(sdp)) == JUICE_ERR_SUCCESS)
		{
			m_LocalSessionToken = std::string(sdp);
			CH_CORE_INFO("[Network][ICE] Candidate gathering completed. Ready for connection.");
		}
	}

	void JuiceIceDriver::HandleRecv(juice_agent_t* /*agent*/, const uint8_t* data, size_t size)
	{
		if (size < 1 || !data)
		{
			return;
		}

		uint8_t channel = data[0];
		const uint8_t* payload = data + 1;
		size_t payloadLen = size - 1;

		NetworkDriverEvent ev;
		ev.Type = NetworkDriverEventType::PacketReceived;
		ev.PeerIndex = 0;
		ev.Channel = channel;
		ev.Data.assign(payload, payload + payloadLen);

		std::lock_guard<std::mutex> lock(m_InboundMutex);
		m_InboundQueue.push_back(std::move(ev));
	}

} // namespace Chained
