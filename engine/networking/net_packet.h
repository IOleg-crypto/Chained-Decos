#ifndef CH_NET_PACKET_H
#define CH_NET_PACKET_H

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
		RPC,
		PlayerInfo,
		PlayerList,
		ChatMessage
	};

	enum class PacketFlags : uint32_t
	{
		Unreliable = 0,
		Reliable = 1,
		Unsequenced = 2,
	};

	// ActionFlags bitmask for InputStatePacket
	enum InputAction : uint8_t
	{
		InputAction_None = 0,
		InputAction_Jump = 1 << 0,
		InputAction_Sprint = 1 << 1,
		InputAction_Interact = 1 << 2,
	};

	// Sent from client -> server every frame (unreliable)
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
		uint8_t SkinIndex = 0;
		uint8_t IsHost = 0;
	};

	// Sent from host -> all clients when starting the game
	struct SceneChangePacket
	{
		char ScenePath[256] = {};

		static constexpr size_t WireSize()
		{
			return sizeof(SceneChangePacket);
		}

		void Serialize(uint8_t* out) const
		{
			std::memcpy(out, this, sizeof(SceneChangePacket));
		}

		static SceneChangePacket Deserialize(const uint8_t* in)
		{
			SceneChangePacket pkt;
			std::memcpy(&pkt, in, sizeof(SceneChangePacket));
			return pkt;
		}
	};

	// Sent from client -> host when connecting (reliable)
	struct PlayerInfoPacket
	{
		char Name[32] = {};
		uint8_t SkinIndex = 0;

		static constexpr size_t WireSize()
		{
			return sizeof(PlayerInfoPacket);
		}

		void Serialize(uint8_t* out) const
		{
			std::memcpy(out, this, sizeof(PlayerInfoPacket));
		}

		static PlayerInfoPacket Deserialize(const uint8_t* in)
		{
			PlayerInfoPacket pkt;
			std::memcpy(&pkt, in, sizeof(PlayerInfoPacket));
			return pkt;
		}
	};

	// Per-entry in a PlayerListPacket (variable-length array follows the header)
	struct PlayerListEntry
	{
		uint64_t NetworkID = 0;
		char Name[32] = {};
		uint8_t SkinIndex = 0;
		uint8_t IsHost = 0;
	};

	// Sent from host -> all clients with the full player list (reliable)
	struct PlayerListPacket
	{
		uint8_t Count = 0;
		// Followed by Count × PlayerListEntry (not included in sizeof — wire format)

		static constexpr size_t HeaderSize()
		{
			return sizeof(uint8_t);
		}

		void Serialize(uint8_t* out) const
		{
			std::memcpy(out, this, HeaderSize());
		}

		static void DeserializeHeader(const uint8_t* in, uint8_t& outCount)
		{
			std::memcpy(&outCount, in, sizeof(uint8_t));
		}
	};

	// Sent from host -> clients when a networked entity appears. PrefabPath is
	// relative to the assets directory; the client instantiates it and tags the
	// result with NetworkID so world state updates can find it.
	struct EntitySpawnPacket
	{
		uint64_t NetworkID = 0;
		char PrefabPath[128] = {};

		static constexpr size_t WireSize()
		{
			return sizeof(EntitySpawnPacket);
		}

		void Serialize(uint8_t* out) const
		{
			std::memcpy(out, this, sizeof(EntitySpawnPacket));
		}

		static EntitySpawnPacket Deserialize(const uint8_t* in)
		{
			EntitySpawnPacket pkt;
			std::memcpy(&pkt, in, sizeof(EntitySpawnPacket));
			return pkt;
		}
	};

	// Sent from host -> clients when a networked entity goes away.
	struct EntityDestroyPacket
	{
		uint64_t NetworkID = 0;

		static constexpr size_t WireSize()
		{
			return sizeof(EntityDestroyPacket);
		}

		void Serialize(uint8_t* out) const
		{
			std::memcpy(out, this, sizeof(EntityDestroyPacket));
		}

		static EntityDestroyPacket Deserialize(const uint8_t* in)
		{
			EntityDestroyPacket pkt;
			std::memcpy(&pkt, in, sizeof(EntityDestroyPacket));
			return pkt;
		}
	};

	// Sent from host -> a single client, telling it which NetworkID is its own.
	// Without this a client cannot tell its own avatar from everyone else's.
	struct PlayerAssignPacket
	{
		uint64_t NetworkID = 0;

		static constexpr size_t WireSize()
		{
			return sizeof(PlayerAssignPacket);
		}

		void Serialize(uint8_t* out) const
		{
			std::memcpy(out, this, sizeof(PlayerAssignPacket));
		}

		static PlayerAssignPacket Deserialize(const uint8_t* in)
		{
			PlayerAssignPacket pkt;
			std::memcpy(&pkt, in, sizeof(PlayerAssignPacket));
			return pkt;
		}
	};

	// Sent from any participant -> all (reliable)
	struct ChatMessagePacket
	{
		uint64_t SenderNetworkID = 0;
		char SenderName[32] = {};
		char Message[256] = {};

		static constexpr size_t WireSize()
		{
			return sizeof(ChatMessagePacket);
		}

		void Serialize(uint8_t* out) const
		{
			std::memcpy(out, this, sizeof(ChatMessagePacket));
		}

		static ChatMessagePacket Deserialize(const uint8_t* in)
		{
			ChatMessagePacket pkt;
			std::memcpy(&pkt, in, sizeof(ChatMessagePacket));
			return pkt;
		}
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
