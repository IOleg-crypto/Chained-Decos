#ifndef CH_NETWORK_SERVICE_H
#define CH_NETWORK_SERVICE_H

#include "net_packet.h"
#include "engine/core/service.h"
#include <string>
#include <vector>

namespace Chained
{

	enum class Role
	{
		Offline = 0,
		Host = 1,
		Client = 2
	};

	class Network : public Service
	{
	public:
		Network() = default;
		~Network() override;

		void Initialize() override;
		void Shutdown() override;

		void HostGame(uint16_t port, int maxClients);
		void ConnectTo(const std::string& ip, uint16_t port);
		void Disconnect();

		void Update(float dt);

		void SendPacket(ENetPeer* peer, PacketType type, const void* data, size_t size);
		void BroadcastPacket(PacketType type, const void* data, size_t size);
		void SendToServer(PacketType type, const void* data, size_t size);

		Role GetRole() const
		{
			return m_Role;
		}
		bool IsHost() const
		{
			return m_Role == Role::Host;
		}
		bool IsClient() const
		{
			return m_Role == Role::Client;
		}
		bool IsConnected() const
		{
			return m_Role != Role::Offline && m_Host != nullptr;
		}

		size_t GetClientCount() const
		{
			return m_Clients.size();
		}
		ENetPeer* GetServerPeer() const
		{
			return m_ServerPeer;
		}

	private:
		Role m_Role = Role::Offline;
		ENetHost* m_Host = nullptr;
		ENetPeer* m_ServerPeer = nullptr;
		std::vector<ENetPeer*> m_Clients;
	};

} // namespace Chained

#endif /* CH_NETWORK_SERVICE_H */
