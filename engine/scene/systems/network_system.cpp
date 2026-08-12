#include "network_system.h"
#include "engine/scene/components/network_identity_component.h"
#include "engine/scene/components/transform_component.h"
#include "engine/scene/components/player_component.h"
#include "engine/scene/components/physics_component.h"
#include "engine/scene/components/camera_component.h"
#include "engine/scene/components/spawn_component.h"
#include "engine/scene/components/scripting_components.h"
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

	NetworkSystem& NetworkSystem::GetInstance()
	{
		static NetworkSystem s_Instance;
		return s_Instance;
	}

	// ---- Peer mapping ----

	void NetworkSystem::SetPlayerPrefab(const std::string& path)
	{
		m_PlayerPrefab = path;
	}

	const std::string& NetworkSystem::GetPlayerPrefab()
	{
		return m_PlayerPrefab;
	}

	void NetworkSystem::RegisterPeerEntity(NetworkPeerHandle peer, uint64_t networkID)
	{
		m_PeerToNetworkID[peer] = networkID;
	}

	void NetworkSystem::UnregisterPeer(NetworkPeerHandle peer)
	{
		m_PeerToNetworkID.erase(peer);
	}

	// ---- Incoming packet processing (client side) ----

	void NetworkSystem::ProcessWorldStatePacket(const uint8_t* data, size_t size)
	{
		if (size < sizeof(uint32_t))
		{
			return;
		}

		uint32_t count = 0;
		std::memcpy(&count, data, sizeof(uint32_t));

		size_t offset = sizeof(uint32_t);
		constexpr size_t PerEntity = sizeof(uint64_t) + sizeof(float) * 7 + sizeof(float) * 3;

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

			std::memcpy(&state.TargetVelocity, data + offset, sizeof(float) * 3);
			offset += sizeof(float) * 3;

			m_PendingStates[state.NetworkID] = state;
		}
	}

	void NetworkSystem::ProcessSceneChangePacket(const uint8_t* data, size_t size)
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

	// Find a spawn position for a new networked entity.
	// Priority: active SpawnComponent entity > scene Player entity > default.
	static glm::vec3 FindSpawnPosition(entt::registry& reg)
	{
		for (auto [entity, spawn] : reg.view<SpawnComponent>().each())
		{
			if (spawn.IsActive)
			{
				if (auto* transform = reg.try_get<TransformComponent>(entity))
				{
					return transform->Translation + spawn.SpawnPoint;
				}
			}
		}
		// Fallback: use scene player entity position
		for (auto [entity, player] : reg.view<PlayerComponent>().each())
		{
			if (auto* transform = reg.try_get<TransformComponent>(entity))
			{
				return transform->Translation;
			}
		}
		return {0, 100, 0};
	}

	void NetworkSystem::ProcessEntitySpawnPacket(const uint8_t* data, size_t size, Scene* scene)
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
		if (m_NetworkIDToEntity.find(pkt.NetworkID) != m_NetworkIDToEntity.end())
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

		// Place only the client's OWN avatar at the spawn point.
		if (m_LocalNetworkID != 0 && pkt.NetworkID == m_LocalNetworkID)
		{
			if (avatar.HasComponent<TransformComponent>())
			{
				auto& transform = avatar.GetComponent<TransformComponent>();
				transform.Translation = FindSpawnPosition(scene->GetRegistry());
				transform.TransformChanged = true;
			}
		}

		auto& netID = avatar.AddOrReplaceComponent<NetworkIdentityComponent>();
		netID.NetworkID = pkt.NetworkID;
		if (m_LocalNetworkID != 0)
		{
			netID.IsOwner = (pkt.NetworkID == m_LocalNetworkID);
		}
		else
		{
			netID.IsOwner = false;
			CH_CORE_WARN("Network: EntitySpawn for netID={} arrived before PlayerAssign — IsOwner deferred.",
						 pkt.NetworkID);
		}

		m_NetworkIDToEntity[pkt.NetworkID] = avatar.GetUUID();
		CH_CORE_INFO("Network: Spawned replicated entity (netID={}, owner={}).", pkt.NetworkID, netID.IsOwner);
	}

	void NetworkSystem::ProcessEntityDestroyPacket(const uint8_t* data, size_t size, Scene* scene)
	{
		if (size < EntityDestroyPacket::WireSize() || !scene)
		{
			return;
		}

		EntityDestroyPacket pkt = EntityDestroyPacket::Deserialize(data);
		auto it = m_NetworkIDToEntity.find(pkt.NetworkID);
		if (it == m_NetworkIDToEntity.end())
		{
			return;
		}

		Entity entity = scene->GetEntityByUUID(it->second);
		if (entity)
		{
			scene->DestroyEntity(entity);
		}

		m_NetworkIDToEntity.erase(it);
		m_PendingStates.erase(pkt.NetworkID);
		CH_CORE_INFO("Network: Destroyed replicated entity (netID={}).", pkt.NetworkID);
	}

	// Host tells the client which NetworkID belongs to it.
	void NetworkSystem::ProcessPlayerAssignPacket(const uint8_t* data, size_t size)
	{
		if (size < PlayerAssignPacket::WireSize())
		{
			return;
		}

		PlayerAssignPacket pkt = PlayerAssignPacket::Deserialize(data);
		m_LocalNetworkID = pkt.NetworkID;

		if (auto* net = ServiceLocator::TryGet<Network>())
		{
			net->SetLocalNetworkID(pkt.NetworkID);
		}

		// A spawn for this entity may already have arrived; correct its ownership.
		auto it = m_NetworkIDToEntity.find(pkt.NetworkID);
		if (it != m_NetworkIDToEntity.end())
		{
			if (auto* scene = m_ReplicationScene)
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

	void NetworkSystem::ProcessInputStatePacket(const uint8_t* data, size_t size, NetworkPeerHandle sender)
	{
		if (size < InputStatePacket::WireSize())
		{
			return;
		}

		InputStatePacket pkt = InputStatePacket::Deserialize(data);

		// Resolve peer -> NetworkID
		auto it = m_PeerToNetworkID.find(sender);
		uint64_t networkID = (it != m_PeerToNetworkID.end()) ? it->second : 0;
		if (networkID == 0)
		{
			if (auto* net = ServiceLocator::TryGet<Network>())
			{
				networkID = net->GetNetworkIDForConnection(sender);
				if (networkID != 0)
				{
					m_PeerToNetworkID[sender] = networkID;
				}
			}
		}

		ProcessedInput input;
		input.NetworkID = networkID;
		input.MoveX = pkt.MoveX;
		input.MoveZ = pkt.MoveZ;
		input.ActionFlags = pkt.ActionFlags;
		input.MouseX = pkt.MouseX;
		input.MouseY = pkt.MouseY;

		m_PendingInputs.push_back(input);
	}

	// Host processes PlayerInfo from a connecting client.
	void NetworkSystem::ProcessPlayerInfoPacket(const uint8_t* data, size_t size, NetworkPeerHandle sender)
	{
		if (size < PlayerInfoPacket::WireSize())
		{
			return;
		}

		PlayerInfoPacket pkt = PlayerInfoPacket::Deserialize(data);

		auto* net = ServiceLocator::TryGet<Network>();
		if (!net)
		{
			return;
		}

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
	void NetworkSystem::ProcessPlayerListPacket(const uint8_t* data, size_t size)
	{
		if (size < PlayerListPacket::HeaderSize())
		{
			return;
		}

		uint8_t count = 0;
		PlayerListPacket::DeserializeHeader(data, count);

		size_t entrySize = sizeof(PlayerListEntry);
		size_t offset = PlayerListPacket::HeaderSize();

		auto* net = ServiceLocator::TryGet<Network>();
		if (!net)
		{
			return;
		}

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
	void NetworkSystem::ProcessChatMessagePacket(const uint8_t* data, size_t size)
	{
		if (size < ChatMessagePacket::WireSize())
		{
			return;
		}

		ChatMessagePacket pkt = ChatMessagePacket::Deserialize(data);
		m_PendingChatMessages.push_back(pkt);
		CH_CORE_INFO("Network: Chat from '{}': {}", pkt.SenderName, pkt.Message);
	}

	// ---- Host-side: apply inputs from remote clients ----

	void NetworkSystem::ApplyHostInputs(entt::registry& reg, Timestep ts)
	{
		if (m_PendingInputs.empty())
		{
			return;
		}

		float dt = static_cast<float>(ts);

		for (auto& input : m_PendingInputs)
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

				if (rb.Handle == kInvalidPhysicsBody)
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

		m_PendingInputs.clear();
	}

	// ---- Client-side: collect local input and send to server ----

	void NetworkSystem::CollectAndSendInput(Network* net, float dt)
	{
		if (!net->IsClient())
		{
			return;
		}

		InputStatePacket pkt;
		pkt.Tick = m_ClientTick++;
		pkt.DeltaTime = dt;

		// WASD movement direction — camera-relative
		float rawX = 0.0f;
		float rawZ = 0.0f;
		if (Core::Input::IsKeyDown(KeyCode::W))
		{
			rawZ += 1.0f;
		}
		if (Core::Input::IsKeyDown(KeyCode::S))
		{
			rawZ -= 1.0f;
		}
		if (Core::Input::IsKeyDown(KeyCode::A))
		{
			rawX -= 1.0f;
		}
		if (Core::Input::IsKeyDown(KeyCode::D))
		{
			rawX += 1.0f;
		}

		// Transform to camera-relative space
		float moveX = rawX;
		float moveZ = rawZ;
		if (m_ReplicationScene)
		{
			auto camView = m_ReplicationScene->GetRegistry().view<CameraComponent, TransformComponent>();
			for (auto entity : camView)
			{
				auto& cam = camView.get<CameraComponent>(entity);
				if (!cam.Primary)
				{
					continue;
				}
				auto& tc = camView.get<TransformComponent>(entity);
				glm::vec3 forward = tc.WorldTransform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
				forward.y = 0.0f;
				float fwdLen = glm::length(forward);
				if (fwdLen > 0.001f)
				{
					forward /= fwdLen;
				}
				else
				{
					forward = glm::vec3(0.0f, 0.0f, -1.0f);
				}
				glm::vec3 right = glm::vec3(-forward.z, 0.0f, forward.x);

				moveX = rawX * right.x + rawZ * forward.x;
				moveZ = rawX * right.z + rawZ * forward.z;
				break;
			}
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

	void NetworkSystem::InterpolateEntities(entt::registry& reg, float dt)
	{
		constexpr float InterpSpeed = 15.0f;
		float t = glm::clamp(dt * InterpSpeed, 0.0f, 1.0f);

		auto view = reg.view<NetworkIdentityComponent, TransformComponent>();
		for (auto entity : view)
		{
			auto& netID = view.get<NetworkIdentityComponent>(entity);
			auto& transform = view.get<TransformComponent>(entity);

			auto it = m_PendingStates.find(netID.NetworkID);
			if (it == m_PendingStates.end())
			{
				continue;
			}

			auto& target = it->second;

			// --- Owned avatar: local simulation runs physics, server only corrects ---
			if (netID.IsOwner)
			{
				float dist = glm::length(transform.Translation - target.TargetPosition);

				if (dist > NetworkSystem::kMaxCorrectionDistance)
				{
					// SNAP: client diverged too far from authoritative state.
					transform.Translation = target.TargetPosition;
					transform.RotationQuat = glm::normalize(target.TargetRotation);
					transform.Rotation = glm::eulerAngles(transform.RotationQuat);
					transform.TransformChanged = true;

					if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
					{
						rb->Velocity = target.TargetVelocity;
					}
				}
				continue;
			}

			// --- Remote entities: interpolate from server state (other players) ---
			transform.Translation = glm::mix(transform.Translation, target.TargetPosition, t);

			glm::quat currentQuat = glm::quat_cast(glm::mat4(transform.WorldTransform));
			glm::quat targetQuat = glm::normalize(target.TargetRotation);
			glm::quat blended = glm::slerp(currentQuat, targetQuat, t);
			transform.Rotation = glm::eulerAngles(blended);
			transform.RotationQuat = blended;
			transform.TransformChanged = true;

			if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
			{
				rb->Velocity = target.TargetVelocity;
			}
		}
	}

	// ---- Host-side: broadcast world state ----

	void NetworkSystem::BroadcastWorldState(entt::registry& reg)
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
			glm::vec3 Velocity;
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
			s.Velocity = reg.try_get<RigidBodyComponent>(entity) ? reg.get<RigidBodyComponent>(entity).Velocity
																 : glm::vec3{0, 0, 0};
			states.push_back(s);
		}

		constexpr size_t Header = sizeof(uint32_t);
		constexpr size_t PerEntity = sizeof(uint64_t) + sizeof(float) * 7 + sizeof(float) * 3;
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

			std::memcpy(buffer.data() + offset, &s.Velocity, sizeof(float) * 3);
			offset += sizeof(float) * 3;
		}

		net->BroadcastPacket(PacketType::WorldState, buffer.data(), totalSize, false);
	}

	// ---- Host-side: broadcast an entity spawn/destroy ----

	void NetworkSystem::SendEntitySpawn(Network* net, uint64_t networkID, const std::string& prefabPath,
										NetworkPeerHandle to)
	{
		EntitySpawnPacket pkt;
		pkt.NetworkID = networkID;
		std::strncpy(pkt.PrefabPath, prefabPath.c_str(), sizeof(pkt.PrefabPath) - 1);
		pkt.PrefabPath[sizeof(pkt.PrefabPath) - 1] = '\0';

		uint8_t buffer[EntitySpawnPacket::WireSize()];
		pkt.Serialize(buffer);

		if (to == kInvalidPeerHandle)
		{
			net->BroadcastPacket(PacketType::EntitySpawn, buffer, sizeof(buffer), true);
		}
		else
		{
			net->SendPacket(to, PacketType::EntitySpawn, buffer, sizeof(buffer), true);
		}
	}

	void NetworkSystem::SendEntityDestroy(Network* net, uint64_t networkID)
	{
		EntityDestroyPacket pkt;
		pkt.NetworkID = networkID;

		uint8_t buffer[EntityDestroyPacket::WireSize()];
		pkt.Serialize(buffer);
		net->BroadcastPacket(PacketType::EntityDestroy, buffer, sizeof(buffer), true);
	}

	// The host's own player is authored in the scene, not spawned from a prefab, so
	// it carries no network identity. Attach one at session start (reserved ID 1)
	// so the host shows up in everyone else's world.
	void NetworkSystem::EnsureHostIdentity(Scene* scene)
	{
		if (!scene || scene->GetSettings().Type == SceneType::UI)
		{
			return;
		}

		entt::registry& reg = scene->GetRegistry();

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

			// Move host player to the scene's spawn point
			if (reg.all_of<TransformComponent>(entity))
			{
				auto& transform = reg.get<TransformComponent>(entity);
				transform.Translation = FindSpawnPosition(reg);
				transform.TransformChanged = true;
			}

			CH_CORE_INFO("Network: Tagged host player with netID={}.", HostAvatarNetworkID);
			return;
		}

		// No static player entity in scene — spawn host avatar dynamically from player prefab
		if (!m_PlayerPrefab.empty())
		{
			std::string path = m_PlayerPrefab;
			if (auto* am = ServiceLocator::TryGet<AssetManager>())
			{
				path = am->ResolvePath(m_PlayerPrefab);
			}

			Entity hostAvatar = PrefabSerializer::Deserialize(scene, path);
			if (hostAvatar)
			{
				if (hostAvatar.HasComponent<TransformComponent>())
				{
					auto& transform = hostAvatar.GetComponent<TransformComponent>();
					transform.Translation = FindSpawnPosition(reg);
					transform.TransformChanged = true;
				}

				auto& netID = hostAvatar.AddOrReplaceComponent<NetworkIdentityComponent>();
				netID.NetworkID = HostAvatarNetworkID;
				netID.IsOwner = true;

				bool hasRB = hostAvatar.HasComponent<RigidBodyComponent>();
				bool hasCollider = hostAvatar.HasComponent<ColliderComponent>();
				int rbType = hasRB ? (int)hostAvatar.GetComponent<RigidBodyComponent>().Type : -1;
				CH_CORE_INFO(
					"Network: Dynamically spawned host avatar (netID={}, entity={}, rb={}, collider={}, rbType={})",
					HostAvatarNetworkID, (uint32_t)hostAvatar, hasRB, hasCollider, rbType);
			}
		}
	}

	// ---- Host-side: spawn/despawn an avatar per connected peer ----

	void NetworkSystem::SyncPeerAvatars(Scene* scene, Network* net)
	{
		if (!scene || scene->GetSettings().Type == SceneType::UI)
		{
			return;
		}

		if (m_PlayerPrefab.empty())
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
		for (NetworkPeerHandle peer : clients)
		{
			if (m_PeerToAvatar.find(peer) != m_PeerToAvatar.end())
			{
				continue;
			}

			uint64_t networkID = net->GetNetworkIDForConnection(peer);
			if (networkID == 0)
			{
				continue; // not accepted yet — retry next frame
			}
			m_PeerToNetworkID[peer] = networkID;

			std::string path = m_PlayerPrefab;
			if (auto* am = ServiceLocator::TryGet<AssetManager>())
			{
				path = am->ResolvePath(m_PlayerPrefab);
			}

			Entity avatar = PrefabSerializer::Deserialize(scene, path);
			if (!avatar)
			{
				CH_CORE_ERROR("Network: Failed to spawn player prefab '{}' for peer {}.", m_PlayerPrefab,
							  (uint32_t)peer);
				m_PeerToAvatar[peer] = UUID(0);
				continue;
			}

			// Place the avatar at the scene's spawn point with an offset per peer
			if (avatar.HasComponent<TransformComponent>())
			{
				auto& transform = avatar.GetComponent<TransformComponent>();
				glm::vec3 spawnPos = FindSpawnPosition(scene->GetRegistry());
				spawnPos.x += static_cast<float>(networkID - 1) * 1.5f;
				transform.Translation = spawnPos;
				transform.TransformChanged = true;
			}

			auto& netID = avatar.AddOrReplaceComponent<NetworkIdentityComponent>();
			netID.NetworkID = networkID;
			netID.IsOwner = false;

			m_PeerToAvatar[peer] = avatar.GetUUID();
			CH_CORE_INFO("Network: Spawned avatar (netID={}) for peer {}.", networkID, (uint32_t)peer);

			// Catch the newcomer up on everything already replicated
			for (const auto& [existingPeer, uuid] : m_PeerToAvatar)
			{
				if (existingPeer == peer || uuid == UUID(0))
				{
					continue;
				}
				auto netIdIt = m_PeerToNetworkID.find(existingPeer);
				if (netIdIt != m_PeerToNetworkID.end())
				{
					SendEntitySpawn(net, netIdIt->second, m_PlayerPrefab, peer);
				}
			}
			SendEntitySpawn(net, HostAvatarNetworkID, m_PlayerPrefab, peer);
			SendEntitySpawn(net, networkID, m_PlayerPrefab, kInvalidPeerHandle);
		}

		// Despawn avatars whose peer is gone.
		for (auto it = m_PeerToAvatar.begin(); it != m_PeerToAvatar.end();)
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

			auto idIt = m_PeerToNetworkID.find(it->first);
			if (idIt != m_PeerToNetworkID.end())
			{
				SendEntityDestroy(net, idIt->second);
			}

			UnregisterPeer(it->first);
			CH_CORE_INFO("Network: Despawned avatar for peer {}.", (uint32_t)it->first);
			it = m_PeerToAvatar.erase(it);
		}
	}

	void NetworkSystem::InstallPacketCallback()
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || m_CallbackRole == net->GetRole())
		{
			return;
		}

		m_CallbackRole = net->GetRole();
		net->SetPacketCallback(
			[this, net](PacketType type, const uint8_t* data, size_t size, NetworkPeerHandle sender) {
				if (net->IsHost())
				{
					if (type == PacketType::InputState)
					{
						ProcessInputStatePacket(data, size, sender);
					}
					else if (type == PacketType::PlayerInfo)
					{
						ProcessPlayerInfoPacket(data, size, sender);
					}
					else if (type == PacketType::ChatMessage)
					{
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
						ProcessPlayerListPacket(data, size);
					}
					else if (type == PacketType::PlayerAssign)
					{
						ProcessPlayerAssignPacket(data, size);
					}
					else if (type == PacketType::EntitySpawn)
					{
						ProcessEntitySpawnPacket(data, size, m_ReplicationScene);
					}
					else if (type == PacketType::EntityDestroy)
					{
						ProcessEntityDestroyPacket(data, size, m_ReplicationScene);
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

	// ---- Early identity setup (runs BEFORE scripts) ----

	void NetworkSystem::EnsureLocalIdentity(Scene* scene)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || net->GetRole() == Role::Offline || !scene)
		{
			return;
		}

		if (net->IsHost())
		{
			EnsureHostIdentity(scene);
		}
	}

	// ---- Client-side: interpolate towards server state ----

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
		if (m_ReplicationScene != scene)
		{
			m_ReplicationScene = scene;
			m_NetworkIDToEntity.clear();
			m_PeerToAvatar.clear();
			m_PendingStates.clear();
		}

		InstallPacketCallback();

		float dt = static_cast<float>(ts);

		// Pump network events (process connect/disconnect/receive)
		net->Update(dt);

		if (net->IsHost())
		{
			EnsureHostIdentity(scene);
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
		return m_PendingInputs;
	}

	void NetworkSystem::ClearPendingInputs()
	{
		m_PendingInputs.clear();
	}

	const std::vector<ChatMessagePacket>& NetworkSystem::GetPendingChatMessages()
	{
		return m_PendingChatMessages;
	}

	void NetworkSystem::ClearPendingChatMessages()
	{
		m_PendingChatMessages.clear();
	}

} // namespace Chained
