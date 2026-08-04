#include "script_glue_network.h"
#include "engine/networking/network_service.h"
#include "engine/core/service_locator.h"

namespace Chained
{

	CH_SCRIPT_FUNC void Network_HostGame(uint16_t port, int maxClients)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net)
		{
			return;
		}
		CH_CORE_INFO("[Script] Network.HostGame(port={}, maxClients={})", port, maxClients);
		net->HostGame(port, maxClients);
	}

	CH_SCRIPT_FUNC void Network_ConnectTo(const Coral::UCChar* ip, uint16_t port)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || !ip)
		{
			return;
		}
		std::string ipStr = ch_u16_to_string(ip);
		CH_CORE_INFO("[Script] Network.ConnectTo(ip='{}', port={})", ipStr, port);
		net->ConnectTo(ipStr, port);
	}

	CH_SCRIPT_FUNC void Network_Disconnect()
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net)
		{
			return;
		}
		CH_CORE_INFO("[Script] Network.Disconnect()");
		net->Disconnect();
	}

	CH_SCRIPT_FUNC bool Network_IsHost()
	{
		auto* net = ServiceLocator::TryGet<Network>();
		return net && net->IsHost();
	}

	CH_SCRIPT_FUNC bool Network_IsClient()
	{
		auto* net = ServiceLocator::TryGet<Network>();
		return net && net->IsClient();
	}

	CH_SCRIPT_FUNC bool Network_IsConnected()
	{
		auto* net = ServiceLocator::TryGet<Network>();
		return net && net->IsConnected();
	}

	CH_SCRIPT_FUNC int Network_GetClientCount()
	{
		auto* net = ServiceLocator::TryGet<Network>();
		return net ? static_cast<int>(net->GetClientCount()) : 0;
	}

	CH_SCRIPT_FUNC int Network_GetRole()
	{
		auto* net = ServiceLocator::TryGet<Network>();
		return net ? static_cast<int>(net->GetRole()) : 0;
	}

} // namespace Chained
