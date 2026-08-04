#ifndef SCRIPT_GLUE_NETWORK_H
#define SCRIPT_GLUE_NETWORK_H
#include "script_glue_internal.h"

namespace Chained
{
	class Network;
}

namespace Chained
{

	CH_SCRIPT_FUNC void Network_HostGame(uint16_t port, int maxClients);

	CH_SCRIPT_FUNC void Network_ConnectTo(const Coral::UCChar* ip, uint16_t port);

	CH_SCRIPT_FUNC void Network_Disconnect();

	CH_SCRIPT_FUNC bool Network_IsHost();

	CH_SCRIPT_FUNC bool Network_IsClient();

	CH_SCRIPT_FUNC bool Network_IsConnected();

	CH_SCRIPT_FUNC int Network_GetClientCount();

	CH_SCRIPT_FUNC int Network_GetRole();

} // namespace Chained
#endif
