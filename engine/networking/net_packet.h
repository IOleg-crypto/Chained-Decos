#ifndef CH_NET_PACKET_H
#define CH_NET_PACKET_H

#include <bit>
#include <cstdint>
#include <cstring>

static_assert(std::endian::native == std::endian::little, "Network packets assume little-endian byte order. "
														  "Add byte-swapping for big-endian platforms.");

namespace Chained
{
	// Protocol version — bump when wire format changes.
	// Receivers silently drop packets with a mismatched version.
	static constexpr uint8_t kProtocolVersion = 1;

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

	// ── Template-based serialization ─────────────────────────────────────
	// Replaces the copy-pasted WireSize/Serialize/Deserialize pattern.
	// Specialize for packets that need null-termination or custom layout.

	template <typename T> struct PacketTraits
	{
		static constexpr size_t WireSize()
		{
			return sizeof(T);
		}

		static void Serialize(const T& pkt, uint8_t* out)
		{
			std::memcpy(out, &pkt, sizeof(T));
		}

		static T Deserialize(const uint8_t* in)
		{
			T pkt;
			std::memcpy(&pkt, in, sizeof(T));
			return pkt;
		}
	};

	// ── Packet header (prepended to every datagram) ──────────────────────

	struct PacketHeader
	{
		uint8_t Version = kProtocolVersion;
		PacketType Type = PacketType::None;

		static constexpr size_t WireSize()
		{
			return sizeof(PacketHeader);
		}

		void Serialize(uint8_t* out) const
		{
			std::memcpy(out, this, sizeof(PacketHeader));
		}

		static PacketHeader Deserialize(const uint8_t* in)
		{
			PacketHeader hdr;
			std::memcpy(&hdr, in, sizeof(PacketHeader));
			return hdr;
		}
	};

	// ── Concrete packet types ────────────────────────────────────────────

#pragma pack(push, 1)

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

		using Traits = PacketTraits<InputStatePacket>;
		static constexpr size_t WireSize()
		{
			return Traits::WireSize();
		}
		void Serialize(uint8_t* out) const
		{
			Traits::Serialize(*this, out);
		}
		static InputStatePacket Deserialize(const uint8_t* in)
		{
			return Traits::Deserialize(in);
		}
	};

	// Per-player info for internal engine representations
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
			pkt.ScenePath[sizeof(pkt.ScenePath) - 1] = '\0';
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
			pkt.Name[sizeof(pkt.Name) - 1] = '\0';
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

		static constexpr size_t WireSize()
		{
			return sizeof(uint64_t) + 32 + sizeof(uint8_t) + sizeof(uint8_t); // 42 bytes
		}

		void Serialize(uint8_t* out) const
		{
			std::memcpy(out, &NetworkID, sizeof(NetworkID));
			std::memcpy(out + sizeof(NetworkID), Name, 32);
			out[sizeof(NetworkID) + 32] = SkinIndex;
			out[sizeof(NetworkID) + 33] = IsHost;
		}

		static PlayerListEntry Deserialize(const uint8_t* in)
		{
			PlayerListEntry entry;
			std::memcpy(&entry.NetworkID, in, sizeof(entry.NetworkID));
			std::memcpy(entry.Name, in + sizeof(entry.NetworkID), 32);
			entry.Name[31] = '\0';
			entry.SkinIndex = in[sizeof(entry.NetworkID) + 32];
			entry.IsHost = in[sizeof(entry.NetworkID) + 33];
			return entry;
		}
	};

	// Sent from host -> all clients with the full player list (reliable)
	struct PlayerListPacket
	{
		uint8_t Count = 0;

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

	// Sent from host -> clients when a networked entity appears
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
			pkt.PrefabPath[sizeof(pkt.PrefabPath) - 1] = '\0';
			return pkt;
		}
	};

	// Sent from host -> clients when a networked entity goes away
	struct EntityDestroyPacket
	{
		uint64_t NetworkID = 0;

		using Traits = PacketTraits<EntityDestroyPacket>;
		static constexpr size_t WireSize()
		{
			return Traits::WireSize();
		}
		void Serialize(uint8_t* out) const
		{
			Traits::Serialize(*this, out);
		}
		static EntityDestroyPacket Deserialize(const uint8_t* in)
		{
			return Traits::Deserialize(in);
		}
	};

	// Sent from host -> a single client with its NetworkID assignment
	struct PlayerAssignPacket
	{
		uint64_t NetworkID = 0;

		using Traits = PacketTraits<PlayerAssignPacket>;
		static constexpr size_t WireSize()
		{
			return Traits::WireSize();
		}
		void Serialize(uint8_t* out) const
		{
			Traits::Serialize(*this, out);
		}
		static PlayerAssignPacket Deserialize(const uint8_t* in)
		{
			return Traits::Deserialize(in);
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
			pkt.SenderName[sizeof(pkt.SenderName) - 1] = '\0';
			pkt.Message[sizeof(pkt.Message) - 1] = '\0';
			return pkt;
		}
	};

#pragma pack(pop)

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
