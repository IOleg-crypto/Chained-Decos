#ifndef CH_NETWORK_TYPES_H
#define CH_NETWORK_TYPES_H

#include <cstdint>
#include <string>

namespace Chained
{
	using NetworkPeerHandle = int;
	constexpr NetworkPeerHandle kInvalidPeerHandle = -1;

	enum class Role : uint8_t
	{
		Offline = 0,
		Host,
		Client,
		HostAndClient,
	};

	enum class NetworkError : uint8_t
	{
		None = 0,
		ENetInitFailed,
		AlreadyConnected,
		CreateHostFailed,
		ConnectFailed,
		BindFailed,
	};

	struct ChatMessagePacket
	{
		uint64_t SenderNetworkID = 0;
		std::string SenderName;
		std::string Message;
	};

} // namespace Chained

#endif // CH_NETWORK_TYPES_H
