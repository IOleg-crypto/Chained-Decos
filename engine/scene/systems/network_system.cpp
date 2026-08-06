#include "network_system.h"
#include "engine/scene/components/network_identity_component.h"
#include "engine/scene/components/transform_component.h"
#include "engine/scene/components/player_component.h"
#include "engine/scene/components/physics_component.h"
#include "engine/networking/network_service.h"
#include "engine/scene/scene.h"
#include "engine/scene/prefab_serializer.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/core/input.h"
#include "engine/core/key_codes.h"
#include "engine/core/log.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cstring>

namespace Chained
{

	// Persistent interpolation buffer for client-side state smoothing.
	static std::unordered_map<uint64_t, PendingNetworkState> s_PendingStates;

	// Accumulated inputs from clients (host-side).
	static std::vector<ProcessedInput> s_PendingInputs;

	// Client tick counter.
	static uint32_t s_ClientTick = 0;

	// Peer -> NetworkID mapping (host-side).
	static std::unordered_map<HSteamNetConnection, uint64_t> s_PeerToNetworkID;

	// Role the packet callback was installed for, so it is reinstalled when the role changes.
	static Role s_CallbackRole = Role::Offline;

	// Prefab instantiated per connecting client (host-side). Relative to the assets
	// directory. Empty by default: no player prefab exists on disk yet, and spawning
	// the wrong one silently would be worse than refusing. Set via SetPlayerPrefab().
	static std::string s_PlayerPrefab;

	// Avatar entity spawned for each peer, so it can be destroyed on disconnect.
	static std::unordered_map<HSteamNetConnection, UUID> s_PeerToAvatar;

	// Identity reserved for the host's own player. Network assigns peers from 2 up.
	static constexpr uint64_t HostAvatarNetworkID = 1;

	// Chat messages received from the network (pending for C# consumption).
	static std::vector<ChatMessagePacket> s_PendingChatMessages;

	// Client-side: NetworkID -> locally instantiated entity, so world state and
	// destroy packets resolve without scanning the registry.
	static std::unordered_map<uint64_t, UUID> s_NetworkIDToEntity;

	// Client-side: the identity the host assigned to us (via PlayerAssign). The
	// owned avatar must not be interpolated towards its own echoed state.
	static uint64_t s_LocalNetworkID = 0;

	// Scene the replication maps were built against. A scene change invalidates
	// every entity handle, so the maps are dropped when the scene swaps.
	static Scene* s_ReplicationScene = nullptr;

	void NetworkSystem::SetPlayerPrefab(const std::string& path)
	{
		s_PlayerPrefab = path;
	}

	const std::string& NetworkSystem::GetPlayerPrefab()
	{
		return s_PlayerPrefab;
	}

	// ---- Peer mapping ----

	void NetworkSystem::RegisterPeerEntity(HSteamNetConnection peer, uint64_t networkID)
	{
		s_PeerToNetworkID[peer] = networkID;
	}

	void NetworkSystem::UnregisterPeer(HSteamNetConnection peer)
	{
		s_PeerToNetworkID.erase(peer);
	}

	// ---- Incoming packet processing (client side) ----

	static void ProcessWorldStatePacket(const uint8_t* data, size_t size)
	{
		if (size < sizeof(uint32_t))
		{
			return;
		}

		uint32_t count = 0;
		std::memcpy(&count, data, sizeof(uint32_t));

		size_t offset = sizeof(uint32_t);
		constexpr size_t PerEntity = sizeof(uint64_t) + sizeof(float) * 7;

		for (uint32_t i = 0; i < count; ++i)
		{
			if (offset + PerEntity > size)
			{
				break;
			}

			PendingNetworkState state;
			std::memcpy(&state.NetworkID, data + offset, sizeof(uint64_t));
			offset += sizeof(uint64_t);

			std::memcpy(&state.TargetPosition, data + offset, sizeof(float) * 3);
			offset += sizeof(float) * 3;

			std::memcpy(&state.TargetRotation, data + offset, sizeof(float) * 4);
			offset += sizeof(float) * 4;

			s_PendingStates[state.NetworkID] = state;
		}
	}

	static void ProcessSceneChangePacket(const uint8_t* data, size_t size)
	{
		if (size < sizeof(SceneChangePacket))
		{
			return;
		}

		SceneChangePacket pkt = SceneChangePacket::Deserialize(data);
		if (pkt.ScenePath[0] == '\0')
		{
			return;
		}

		auto* net = ServiceLocator::TryGet<Network>();
		if (net)
		{
			net->SetPendingSceneChange(std::string(pkt.ScenePath));
			CH_CORE_INFO("Network: Received scene change -> {}", pkt.ScenePath);
		}
	}

	// Client instantiates the prefab the host told it about and binds it to the
	// NetworkID so WorldState updates can steer it.
	static void ProcessEntitySpawnPacket(const uint8_t* data, size_t size, Scene* scene)
	{
		if (size < EntitySpawnPacket::WireSize() || !scene)
		{
			return;
		}

		EntitySpawnPacket pkt = EntitySpawnPacket::Deserialize(data);
		if (pkt.NetworkID == 0 || pkt.PrefabPath[0] == '\0')
		{
			return;
		}

		// Idempotent: the host re-sends the full set to late joiners, and the
		// newcomer's own spawn is echoed to it as well.
		if (s_NetworkIDToEntity.find(pkt.NetworkID) != s_NetworkIDToEntity.end())
		{
			return;
		}

		std::string path = pkt.PrefabPath;
		if (auto* am = ServiceLocator::TryGet<AssetManager>())
		{
			path = am->ResolvePath(path);
		}

		Entity avatar = PrefabSerializer::Deserialize(scene, path);
		if (!avatar)
		{
			CH_CORE_ERROR("Network: Failed to spawn replicated prefab '{}' (netID={}).", pkt.PrefabPath, pkt.NetworkID);
			return;
		}

		auto& netID = avatar.AddOrReplaceComponent<NetworkIdentityComponent>();
		netID.NetworkID = pkt.NetworkID;
		// The host simulates every avatar; a client only ever interpolates.
		netID.IsOwner = (pkt.NetworkID == s_LocalNetworkID);

		s_NetworkIDToEntity[pkt.NetworkID] = avatar.GetUUID();
		CH_CORE_INFO("Network: Spawned replicated entity (netID={}, owner={}).", pkt.NetworkID, netID.IsOwner);
	}

	static void ProcessEntityDestroyPacket(const uint8_t* data, size_t size, Scene* scene)
	{
		if (size < EntityDestroyPacket::WireSize() || !scene)
		{
			return;
		}

		EntityDestroyPacket pkt = EntityDestroyPacket::Deserialize(data);
		auto it = s_NetworkIDToEntity.find(pkt.NetworkID);
		if (it == s_NetworkIDToEntity.end())
		{
			return;
		}

		Entity entity = scene->GetEntityByUUID(it->second);
		if (entity)
		{
			scene->DestroyEntity(entity);
		}

		s_NetworkIDToEntity.erase(it);
		s_PendingStates.erase(pkt.NetworkID);
		CH_CORE_INFO("Network: Destroyed replicated entity (netID={}).", pkt.NetworkID);
	}

	// Host tells the client which NetworkID belongs to it.
	static void ProcessPlayerAssignPacket(const uint8_t* data, size_t size, Network* net)
	{
		if (size < PlayerAssignPacket::WireSize())
		{
			return;
		}

		PlayerAssignPacket pkt = PlayerAssignPacket::Deserialize(data);
		s_LocalNetworkID = pkt.NetworkID;
		net->SetLocalNetworkID(pkt.NetworkID);

		// A spawn for this entity may already have arrived; correct its ownership.
		auto it = s_NetworkIDToEntity.find(pkt.NetworkID);
		if (it != s_NetworkIDToEntity.end())
		{
			if (auto* scene = s_ReplicationScene)
			{
				Entity entity = scene->GetEntityByUUID(it->second);
				if (entity && entity.HasComponent<NetworkIdentityComponent>())
				{
					entity.GetComponent<NetworkIdentityComponent>().IsOwner = true;
				}
			}
		}

		CH_CORE_INFO("Network: Assigned local network ID {}.", pkt.NetworkID);
	}

	// ---- Incoming packet processing (host side) ----

	static void ProcessInputStatePacket(const uint8_t* data, size_t size, HSteamNetConnection sender)
	{
		if (size < InputStatePacket::WireSize())
		{
			return;
		}

		InputStatePacket pkt = InputStatePacket::Deserialize(data);

		// Resolve peer -> NetworkID
		auto it = s_PeerToNetworkID.find(sender);
		uint64_t networkID = (it != s_PeerToNetworkID.end()) ? it->second : 0;

		ProcessedInput input;
		input.NetworkID = networkID;
		input.MoveX = pkt.MoveX;
		input.MoveZ = pkt.MoveZ;
		input.ActionFlags = pkt.ActionFlags;
		input.MouseX = pkt.MouseX;
		input.MouseY = pkt.MouseY;

		s_PendingInputs.push_back(input);
	}

	// Host processes PlayerInfo from a connecting client.
	static void ProcessPlayerInfoPacket(const uint8_t* data, size_t size, HSteamNetConnection sender, Network* net)
	{
		if (size < PlayerInfoPacket::WireSize())
		{
			return;
		}

		PlayerInfoPacket pkt = PlayerInfoPacket::Deserialize(data);

		// The identity was bound when the connection was accepted. Without a match
		// the packet is from a peer we no longer track — dropping it is safer than
		// guessing an entry and corrupting somebody else's name.
		const uint64_t networkID = net->GetNetworkIDForConnection(sender);
		if (networkID == 0)
		{
			CH_CORE_WARN("Network: PlayerInfo from unknown peer {} — ignored.", (uint32_t)sender);
			return;
		}

		auto& playerList = net->GetPlayerListMutable();
		for (auto& p : playerList)
		{
			if (p.NetworkID != networkID)
			{
				continue;
			}

			std::strncpy(p.Name, pkt.Name, sizeof(p.Name) - 1);
			p.Name[sizeof(p.Name) - 1] = '\0';
			p.SkinIndex = pkt.SkinIndex;
			CH_CORE_INFO("Network: Updated player info: '{}' (netID={}, skin={}).", p.Name, p.NetworkID,
						 (int)p.SkinIndex);
			break;
		}

		// Broadcast updated list to all clients
		net->BroadcastPlayerList();
	}

	// Client processes the full player list from the host.
	static void ProcessPlayerListPacket(const uint8_t* data, size_t size, Network* net)
	{
		if (size < PlayerListPacket::HeaderSize())
		{
			return;
		}

		uint8_t count = 0;
		PlayerListPacket::DeserializeHeader(data, count);

		size_t entrySize = sizeof(PlayerListEntry);
		size_t offset = PlayerListPacket::HeaderSize();

		auto& playerList = net->GetPlayerListMutable();
		playerList.clear();

		for (uint8_t i = 0; i < count; ++i)
		{
			if (offset + entrySize > size)
			{
				break;
			}

			PlayerListEntry entry;
			std::memcpy(&entry, data + offset, entrySize);
			offset += entrySize;

			PlayerNetInfo info;
			info.NetworkID = entry.NetworkID;
			std::strncpy(info.Name, entry.Name, sizeof(info.Name) - 1);
			info.Name[sizeof(info.Name) - 1] = '\0';
			info.SkinIndex = entry.SkinIndex;
			info.IsHost = entry.IsHost;

			playerList.push_back(info);
		}

		CH_CORE_INFO("Network: Received player list ({} players).", count);
	}

	// Processes a chat message from the network (both host and client).
	static void ProcessChatMessagePacket(const uint8_t* data, size_t size)
	{
		if (size < ChatMessagePacket::WireSize())
		{
			return;
		}

		ChatMessagePacket pkt = ChatMessagePacket::Deserialize(data);
		s_PendingChatMessages.push_back(pkt);
		CH_CORE_INFO("Network: Chat from '{}': {}", pkt.SenderName, pkt.Message);
	}

	// ---- Host-side: apply inputs from remote clients ----

	void NetworkSystem::ApplyHostInputs(entt::registry& reg, Timestep ts)
	{
		if (s_PendingInputs.empty())
		{
			return;
		}

		float dt = static_cast<float>(ts);

		for (auto& input : s_PendingInputs)
		{
			if (input.NetworkID == 0)
			{
				continue;
			}

			// Find entity with matching NetworkIdentityComponent
			entt::entity targetEntity = entt::null;
			auto view = reg.view<NetworkIdentityComponent>();
			for (auto entity : view)
			{
				auto& netID = view.get<NetworkIdentityComponent>(entity);
				if (netID.NetworkID == input.NetworkID)
				{
					targetEntity = entity;
					break;
				}
			}

			if (targetEntity == entt::null || !reg.valid(targetEntity))
			{
				continue;
			}

			// Apply movement if entity has PlayerComponent + RigidBodyComponent
			if (reg.all_of<PlayerComponent, RigidBodyComponent>(targetEntity))
			{
				auto& player = reg.get<PlayerComponent>(targetEntity);
				auto& rb = reg.get<RigidBodyComponent>(targetEntity);

				if (rb.Handle == 0)
				{
					continue;
				}

				float speed = player.MovementSpeed;
				if (input.ActionFlags & InputAction_Sprint)
				{
					speed *= 2.0f;
				}

				// Horizontal movement -- set velocity directly, preserve Y from physics
				float moveX = input.MoveX * speed;
				float moveZ = input.MoveZ * speed;
				rb.Velocity = glm::vec3(moveX, rb.Velocity.y, moveZ);

				// Jump -- requires VelocityForced to bypass Y-override in Physics::Update
				if ((input.ActionFlags & InputAction_Jump) && rb.IsGrounded)
				{
					rb.Velocity.y = player.JumpForce;
					rb.VelocityForced = true;
				}
			}
		}

		s_PendingInputs.clear();
	}

	// ---- Client-side: collect local input and send to server ----

	static void CollectAndSendInput(Network* net, float dt)
	{
		if (!net->IsClient())
		{
			return;
		}

		InputStatePacket pkt;
		pkt.Tick = s_ClientTick++;
		pkt.DeltaTime = dt;

		// WASD movement direction
		float moveX = 0.0f;
		float moveZ = 0.0f;
		if (Core::Input::IsKeyDown(KeyCode::W))
		{
			moveZ -= 1.0f;
		}
		if (Core::Input::IsKeyDown(KeyCode::S))
		{
			moveZ += 1.0f;
		}
		if (Core::Input::IsKeyDown(KeyCode::A))
		{
			moveX -= 1.0f;
		}
		if (Core::Input::IsKeyDown(KeyCode::D))
		{
			moveX += 1.0f;
		}

		// Normalize diagonal movement
		float len = std::sqrt(moveX * moveX + moveZ * moveZ);
		if (len > 0.001f)
		{
			moveX /= len;
			moveZ /= len;
		}

		pkt.MoveX = moveX;
		pkt.MoveZ = moveZ;

		// Action flags
		uint8_t flags = 0;
		if (Core::Input::IsKeyPressed(KeyCode::Space))
		{
			flags |= InputAction_Jump;
		}
		if (Core::Input::IsKeyDown(KeyCode::LeftShift))
		{
			flags |= InputAction_Sprint;
		}
		pkt.ActionFlags = flags;

		// Mouse delta
		glm::vec2 mouseDelta = Core::Input::GetMouseDelta();
		pkt.MouseX = mouseDelta.x;
		pkt.MouseY = mouseDelta.y;

		// Send unreliable (inputs are frequent, stale ones are useless)
		uint8_t buffer[InputStatePacket::WireSize()];
		pkt.Serialize(buffer);
		net->SendToServer(PacketType::InputState, buffer, sizeof(buffer));
	}

	// ---- Client-side: interpolate towards server state ----

	static void InterpolateEntities(entt::registry& reg, float dt)
	{
		constexpr float InterpSpeed = 15.0f;
		float t = glm::clamp(dt * InterpSpeed, 0.0f, 1.0f);

		auto view = reg.view<NetworkIdentityComponent, TransformComponent>();
		for (auto entity : view)
		{
			auto& netID = view.get<NetworkIdentityComponent>(entity);
			auto& transform = view.get<TransformComponent>(entity);

			// Every replicated entity is interpolated, including the owned avatar:
			// the client does not simulate locally, so the host's state is the only
			// source of motion. IsOwner marks identity (camera, HUD), not authority.
			auto it = s_PendingStates.find(netID.NetworkID);
			if (it == s_PendingStates.end())
			{
				continue;
			}

			auto& target = it->second;
			transform.Translation = glm::mix(transform.Translation, target.TargetPosition, t);

			glm::quat currentQuat = glm::quat_cast(glm::mat4(transform.WorldTransform));
			glm::quat targetQuat = glm::normalize(target.TargetRotation);
			glm::quat blended = glm::slerp(currentQuat, targetQuat, t);
			transform.Rotation = glm::eulerAngles(blended);
			transform.RotationQuat = blended;
			transform.TransformChanged = true;
		}
	}

	// ---- Host-side: broadcast world state ----

	static void BroadcastWorldState(entt::registry& reg)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || !net->IsHost())
		{
			return;
		}

		struct EntityNetState
		{
			uint64_t NetworkID;
			glm::vec3 Position;
			glm::quat Rotation;
		};

		std::vector<EntityNetState> states;
		auto view = reg.view<NetworkIdentityComponent, TransformComponent>();
		for (auto entity : view)
		{
			auto& netID = view.get<NetworkIdentityComponent>(entity);
			auto& transform = view.get<TransformComponent>(entity);

			EntityNetState s;
			s.NetworkID = netID.NetworkID;
			s.Position = transform.Translation;
			s.Rotation = glm::quat_cast(transform.WorldTransform);
			states.push_back(s);
		}

		constexpr size_t Header = sizeof(uint32_t);
		constexpr size_t PerEntity = sizeof(uint64_t) + sizeof(float) * 7;
		size_t totalSize = Header + states.size() * PerEntity;

		std::vector<uint8_t> buffer(totalSize);
		uint32_t count = static_cast<uint32_t>(states.size());
		std::memcpy(buffer.data(), &count, sizeof(uint32_t));

		size_t offset = Header;
		for (auto& s : states)
		{
			std::memcpy(buffer.data() + offset, &s.NetworkID, sizeof(uint64_t));
			offset += sizeof(uint64_t);

			std::memcpy(buffer.data() + offset, &s.Position, sizeof(float) * 3);
			offset += sizeof(float) * 3;

			std::memcpy(buffer.data() + offset, &s.Rotation, sizeof(float) * 4);
			offset += sizeof(float) * 4;
		}

		net->BroadcastPacket(PacketType::WorldState, buffer.data(), totalSize);
	}

	// ---- Host-side: broadcast an entity spawn/destroy ----

	static void SendEntitySpawn(Network* net, uint64_t networkID, const std::string& prefabPath, HSteamNetConnection to)
	{
		EntitySpawnPacket pkt;
		pkt.NetworkID = networkID;
		std::strncpy(pkt.PrefabPath, prefabPath.c_str(), sizeof(pkt.PrefabPath) - 1);
		pkt.PrefabPath[sizeof(pkt.PrefabPath) - 1] = '\0';

		uint8_t buffer[EntitySpawnPacket::WireSize()];
		pkt.Serialize(buffer);

		if (to == k_HSteamNetConnection_Invalid)
		{
			net->BroadcastPacket(PacketType::EntitySpawn, buffer, sizeof(buffer), true);
		}
		else
		{
			net->SendPacket(to, PacketType::EntitySpawn, buffer, sizeof(buffer), true);
		}
	}

	static void SendEntityDestroy(Network* net, uint64_t networkID)
	{
		EntityDestroyPacket pkt;
		pkt.NetworkID = networkID;

		uint8_t buffer[EntityDestroyPacket::WireSize()];
		pkt.Serialize(buffer);
		net->BroadcastPacket(PacketType::EntityDestroy, buffer, sizeof(buffer), true);
	}

	// The host's own player is authored in the scene, not spawned from a prefab, so
	// it carries no network identity. Attach one at session start (reserved ID 1)
	// so the host shows up in everyone else's world. Scenes and prefabs stay
	// untouched and remain valid for singleplayer.
	static void EnsureHostIdentity(entt::registry& reg)
	{
		auto owned = reg.view<NetworkIdentityComponent>();
		for (auto entity : owned)
		{
			if (owned.get<NetworkIdentityComponent>(entity).NetworkID == HostAvatarNetworkID)
			{
				return; // already tagged
			}
		}

		auto players = reg.view<PlayerComponent>();
		for (auto entity : players)
		{
			if (reg.all_of<NetworkIdentityComponent>(entity))
			{
				continue; // a replicated avatar, not the local player
			}

			auto& netID = reg.emplace<NetworkIdentityComponent>(entity);
			netID.NetworkID = HostAvatarNetworkID;
			netID.IsOwner = true;
			CH_CORE_INFO("Network: Tagged host player with netID={}.", HostAvatarNetworkID);
			return;
		}
	}

	// ---- Host-side: spawn/despawn an avatar per connected peer ----

	static void SyncPeerAvatars(Scene* scene, Network* net)
	{
		if (!scene || scene->GetSettings().Type == SceneType::UI)
		{
			return;
		}

		if (s_PlayerPrefab.empty())
		{
			// Warn once rather than per peer per frame.
			static bool warned = false;
			if (!warned && !net->GetClients().empty())
			{
				warned = true;
				CH_CORE_ERROR("Network: No player prefab configured — clients will connect without an avatar. "
							  "Call NetworkSystem::SetPlayerPrefab().");
			}
			return;
		}

		const auto& clients = net->GetClients();

		// Spawn for peers that don't have an avatar yet.
		for (HSteamNetConnection peer : clients)
		{
			if (s_PeerToAvatar.find(peer) != s_PeerToAvatar.end())
			{
				continue;
			}

			// The identity was bound by Network when the connection was accepted;
			// reuse it so avatar, player list and chat all agree.
			uint64_t networkID = net->GetNetworkIDForConnection(peer);
			if (networkID == 0)
			{
				continue; // not accepted yet — retry next frame
			}
			s_PeerToNetworkID[peer] = networkID;

			std::string path = s_PlayerPrefab;
			if (auto* am = ServiceLocator::TryGet<AssetManager>())
			{
				path = am->ResolvePath(s_PlayerPrefab);
			}

			Entity avatar = PrefabSerializer::Deserialize(scene, path);
			if (!avatar)
			{
				CH_CORE_ERROR("Network: Failed to spawn player prefab '{}' for peer {}.", s_PlayerPrefab,
							  (uint32_t)peer);
				// Record the failure so the prefab isn't reloaded every frame for this peer.
				s_PeerToAvatar[peer] = UUID(0);
				continue;
			}

			auto& netID = avatar.AddOrReplaceComponent<NetworkIdentityComponent>();
			netID.NetworkID = networkID;
			// The host simulates this avatar, but the connected peer owns it.
			netID.IsOwner = false;

			s_PeerToAvatar[peer] = avatar.GetUUID();
			CH_CORE_INFO("Network: Spawned avatar (netID={}) for peer {}.", networkID, (uint32_t)peer);

			// Catch the newcomer up on everything already replicated, then tell
			// everyone (including it) about the new avatar.
			for (const auto& [existingPeer, uuid] : s_PeerToAvatar)
			{
				if (existingPeer == peer || uuid == UUID(0))
				{
					continue;
				}
				auto netIdIt = s_PeerToNetworkID.find(existingPeer);
				if (netIdIt != s_PeerToNetworkID.end())
				{
					SendEntitySpawn(net, netIdIt->second, s_PlayerPrefab, peer);
				}
			}
			SendEntitySpawn(net, HostAvatarNetworkID, s_PlayerPrefab, peer);
			SendEntitySpawn(net, networkID, s_PlayerPrefab, k_HSteamNetConnection_Invalid);
		}

		// Despawn avatars whose peer is gone.
		for (auto it = s_PeerToAvatar.begin(); it != s_PeerToAvatar.end();)
		{
			if (std::find(clients.begin(), clients.end(), it->first) != clients.end())
			{
				++it;
				continue;
			}

			if (it->second != UUID(0))
			{
				Entity avatar = scene->GetEntityByUUID(it->second);
				if (avatar)
				{
					scene->DestroyEntity(avatar);
				}
			}

			auto idIt = s_PeerToNetworkID.find(it->first);
			if (idIt != s_PeerToNetworkID.end())
			{
				SendEntityDestroy(net, idIt->second);
			}

			NetworkSystem::UnregisterPeer(it->first);
			CH_CORE_INFO("Network: Despawned avatar for peer {}.", (uint32_t)it->first);
			it = s_PeerToAvatar.erase(it);
		}
	}

	// ---- Main update ----

	void NetworkSystem::Update(Scene* scene, Timestep ts)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || net->GetRole() == Role::Offline || !scene)
		{
			return;
		}

		entt::registry& reg = scene->GetRegistry();

		// Entity handles do not survive a scene swap, so the replication maps are
		// rebuilt from the host's catch-up spawns rather than carried across.
		if (s_ReplicationScene != scene)
		{
			s_ReplicationScene = scene;
			s_NetworkIDToEntity.clear();
			s_PeerToAvatar.clear();
			s_PendingStates.clear();
		}

		// Install the packet callback for the current role. Reinstalled on role change so
		// a peer that hosts after being a client still dispatches the right packets.
		if (s_CallbackRole != net->GetRole())
		{
			s_CallbackRole = net->GetRole();
			// Capture only net — NOT scene. The scene pointer changes on every scene
			// load and the old captured value becomes dangling. s_ReplicationScene is
			// updated at the top of Update() each frame and is always valid when a
			// packet arrives during the same frame's RunCallbacks() pump.
			net->SetPacketCallback(
				[net](PacketType type, const uint8_t* data, size_t size, HSteamNetConnection sender) {
					if (net->IsHost())
					{
						if (type == PacketType::InputState)
						{
							ProcessInputStatePacket(data, size, sender);
						}
						else if (type == PacketType::PlayerInfo)
						{
							ProcessPlayerInfoPacket(data, size, sender, net);
						}
						else if (type == PacketType::ChatMessage)
						{
							// Host receives from client, stores, and re-broadcasts
							ProcessChatMessagePacket(data, size);
							if (size >= ChatMessagePacket::WireSize())
							{
								ChatMessagePacket pkt = ChatMessagePacket::Deserialize(data);
								net->StorePendingChatMessage(pkt);
							}
							net->BroadcastPacket(PacketType::ChatMessage, data, size, true);
						}
					}
					else if (net->IsClient())
					{
						if (type == PacketType::WorldState)
						{
							ProcessWorldStatePacket(data, size);
						}
						else if (type == PacketType::SceneChange)
						{
							ProcessSceneChangePacket(data, size);
						}
						else if (type == PacketType::PlayerList)
						{
							ProcessPlayerListPacket(data, size, net);
						}
						else if (type == PacketType::PlayerAssign)
						{
							ProcessPlayerAssignPacket(data, size, net);
						}
						else if (type == PacketType::EntitySpawn)
						{
							// Read s_ReplicationScene here — it is updated every frame at
							// the top of NetworkSystem::Update() and is always current.
							ProcessEntitySpawnPacket(data, size, s_ReplicationScene);
						}
						else if (type == PacketType::EntityDestroy)
						{
							ProcessEntityDestroyPacket(data, size, s_ReplicationScene);
						}
						else if (type == PacketType::ChatMessage)
						{
							ProcessChatMessagePacket(data, size);
							if (size >= ChatMessagePacket::WireSize())
							{
								ChatMessagePacket pkt = ChatMessagePacket::Deserialize(data);
								net->StorePendingChatMessage(pkt);
							}
						}
					}
				});
		}

		float dt = static_cast<float>(ts);

		// Pump network events (process connect/disconnect/receive)
		net->Update(dt);

		if (net->IsHost())
		{
			EnsureHostIdentity(reg);
			// Runs after the pump, so peers accepted this frame get an avatar immediately.
			SyncPeerAvatars(scene, net);
			BroadcastWorldState(reg);
		}
		else if (net->IsClient())
		{
			CollectAndSendInput(net, dt);
			InterpolateEntities(reg, dt);
		}
	}

	const std::vector<ProcessedInput>& NetworkSystem::GetPendingInputs()
	{
		return s_PendingInputs;
	}

	void NetworkSystem::ClearPendingInputs()
	{
		s_PendingInputs.clear();
	}

	const std::vector<ChatMessagePacket>& NetworkSystem::GetPendingChatMessages()
	{
		return s_PendingChatMessages;
	}

	void NetworkSystem::ClearPendingChatMessages()
	{
		s_PendingChatMessages.clear();
	}

} // namespace Chained
