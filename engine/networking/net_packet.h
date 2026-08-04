#ifndef CH_NET_PACKET_H
#define CH_NET_PACKET_H

#include <enet.h>
#include <cstdint>
#include <cstring>

namespace Chained
{

	enum class PacketType : uint8_t
	{
		None = 0,
		InputState,
		WorldState,
		EntitySpawn,
		EntityDestroy,
		SceneChange,
		PlayerAssign,
		RPC
	};

	enum class PacketFlags : enet_uint32
	{
		Unreliable = 0,
		Reliable = ENET_PACKET_FLAG_RELIABLE,
		Unsequenced = ENET_PACKET_FLAG_UNSEQUENCED,
		NoAllocate = ENET_PACKET_FLAG_NO_ALLOCATE,
		UnreliableFragment = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT
	};

	// ActionFlags bitmask for InputStatePacket
	enum InputAction : uint8_t
	{
		InputAction_None = 0,
		InputAction_Jump = 1 << 0,
		InputAction_Sprint = 1 << 1,
		InputAction_Interact = 1 << 2,
	};

	// Sent from client → server every frame (unreliable)
	struct InputStatePacket
	{
		uint32_t Tick = 0;
		float MoveX = 0.0f;
		float MoveZ = 0.0f;
		uint8_t ActionFlags = 0;
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		float DeltaTime = 0.0f;

		static constexpr size_t WireSize()
		{
			return sizeof(InputStatePacket);
		}

		void Serialize(uint8_t* out) const
		{
			std::memcpy(out, this, sizeof(InputStatePacket));
		}

		static InputStatePacket Deserialize(const uint8_t* in)
		{
			InputStatePacket pkt;
			std::memcpy(&pkt, in, sizeof(InputStatePacket));
			return pkt;
		}
	};

	// Per-player info for the players list tab
	struct PlayerNetInfo
	{
		uint64_t NetworkID = 0;
		uint32_t Ping = 0;
		char Name[32] = {};
	};

	inline PacketFlags operator|(PacketFlags lhs, PacketFlags rhs)
	{
		using T = std::underlying_type_t<PacketFlags>;
		return static_cast<PacketFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
	}

	inline PacketFlags& operator|=(PacketFlags& lhs, PacketFlags rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	inline PacketFlags operator&(PacketFlags lhs, PacketFlags rhs)
	{
		using T = std::underlying_type_t<PacketFlags>;
		return static_cast<PacketFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
	}

} // namespace Chained

#endif /* CH_NET_PACKET_H */
