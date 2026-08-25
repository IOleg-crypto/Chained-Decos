#ifndef CH_NETWORK_PLAYER_MANAGER_H
#define CH_NETWORK_PLAYER_MANAGER_H

#include "network_types.h"
#include "net_packet.h"

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace Chained
{

	class NetworkPlayerManager
	{
	public:
		NetworkPlayerManager() = default;
		~NetworkPlayerManager() = default;

		NetworkPlayerManager(const NetworkPlayerManager&) = delete;
		NetworkPlayerManager& operator=(const NetworkPlayerManager&) = delete;

		void Reset();

		void SetLocalPlayerInfo(const char* name, uint8_t skinIndex);
		const std::string& GetLocalPlayerName() const
		{
			return m_LocalPlayerName;
		}
		uint8_t GetLocalSkinIndex() const
		{
			return m_LocalSkinIndex;
		}

		uint64_t GetLocalNetworkID() const
		{
			return m_LocalNetworkID;
		}
		void SetLocalNetworkID(uint64_t id)
		{
			m_LocalNetworkID = id;
		}

		void SetHostNetworkID(uint64_t id)
		{
			m_HostNetworkID = id;
		}

		uint64_t GetNetworkIDForConnection(int clientIndex) const;
		void OnClientConnected(int clientIndex);
		void OnClientDisconnected(int clientIndex);

		const std::vector<PlayerNetInfo>& GetPlayerList() const
		{
			return m_PlayerList;
		}
		std::vector<PlayerNetInfo>& GetPlayerListMutable()
		{
			return m_PlayerList;
		}

		void SetPlayerListFromMessage(const std::vector<PlayerNetInfo>& list);
		void UpdatePlayerInfo(uint64_t networkID, const char* name, uint8_t skinIndex);

		uint64_t GenerateClientId();

		size_t GetClientCount() const
		{
			return m_ClientIndexToNetworkID.size();
		}
		std::vector<int> GetClients() const;

		void AddHostSelf(uint64_t hostNetworkID, const std::string& localPlayerName, uint8_t localSkinIndex);

	private:
		std::vector<PlayerNetInfo> m_PlayerList;
		std::unordered_map<int, uint64_t> m_ClientIndexToNetworkID;
		uint64_t m_LocalNetworkID = 0;
		std::string m_LocalPlayerName;
		uint8_t m_LocalSkinIndex = 0;
		uint64_t m_HostNetworkID = 1;
		uint64_t m_NextNetworkID = 2;
	};

} // namespace Chained

#endif // CH_NETWORK_PLAYER_MANAGER_H
