#include "network_system.h"
#include "engine/scene/components/gameplay/network_identity_component.h"
#include "engine/scene/components/core/transform_component.h"
#include "engine/scene/components/core/hierarchy_component.h"
#include "engine/scene/systems/transform_system.h"
#include "engine/scene/components/gameplay/player_component.h"
#include "engine/scene/components/physics/physics_component.h"
#include "engine/scene/components/render/camera_component.h"
#include "engine/scene/components/gameplay/spawn_component.h"
#include "engine/networking/network_service.h"
#include "engine/scene/scene.h"
#include "engine/scene/prefab_serializer.h"
#include "engine/scene/scene_events.h"
#include "engine/app/application.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/core/input.h"
#include "engine/core/key_codes.h"
#include "engine/core/log.h"
#include <glm/gtc/quaternion.hpp>
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

	void NetworkSystem::RegisterPeerEntity(int peer, uint64_t networkID)
	{
		m_PeerToNetworkID[peer] = networkID;
	}

	void NetworkSystem::UnregisterPeer(int peer)
	{
		m_PeerToNetworkID.erase(peer);
	}

	// ---- Incoming message processing (client side) ----

	void NetworkSystem::ProcessWorldStateMessage(WorldStateMessage* msg)
	{
		if (!msg)
		{
			return;
		}

		auto it = m_PendingStates.find(msg->NetworkID);
		if (it != m_PendingStates.end() && msg->Tick < it->second.LastTick &&
			(it->second.LastTick - msg->Tick) < 100000)
		{
			return; // Drop out-of-order / older state
		}

		PendingNetworkState state;
		state.NetworkID = msg->NetworkID;
		state.LastTick = msg->Tick;
		state.TargetPosition = {msg->Position[0], msg->Position[1], msg->Position[2]};
		state.TargetRotation = {msg->Rotation[0], msg->Rotation[1], msg->Rotation[2], msg->Rotation[3]};
		state.TargetVelocity = {msg->Velocity[0], msg->Velocity[1], msg->Velocity[2]};
		state.IsGrounded = (msg->IsGrounded != 0);
		state.ActionFlags = msg->ActionFlags;

		m_PendingStates[state.NetworkID] = state;
	}

	void NetworkSystem::ProcessSceneChangeMessage(SceneChangeMessage* msg)
	{
		if (!msg || msg->ScenePath[0] == '\0')
		{
			return;
		}

		auto* net = ServiceLocator::TryGet<Network>();
		if (net)
		{
			net->SetPendingSceneChange(std::string(msg->ScenePath));
			CH_CORE_INFO("Network: Received scene change -> {}", msg->ScenePath);
		}
	}

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
		for (auto [entity, player] : reg.view<PlayerComponent>().each())
		{
			if (auto* transform = reg.try_get<TransformComponent>(entity))
			{
				return transform->Translation;
			}
		}
		return {0, 100, 0};
	}

	void NetworkSystem::ProcessEntitySpawnMessage(EntitySpawnMessage* msg, Scene* scene)
	{
		if (!msg || !scene)
		{
			return;
		}

		if (msg->NetworkID == 0 || msg->PrefabPath[0] == '\0')
		{
			return;
		}

		if (m_NetworkIDToEntity.find(msg->NetworkID) != m_NetworkIDToEntity.end())
		{
			return;
		}

		std::string path = msg->PrefabPath;
		if (auto* am = ServiceLocator::TryGet<AssetManager>())
		{
			path = am->ResolvePath(path);
		}

		Entity avatar = PrefabSerializer::Deserialize(scene, path);
		if (!avatar)
		{
			CH_CORE_ERROR("Network: Failed to spawn replicated prefab '{}' (netID={}).", msg->PrefabPath,
						  msg->NetworkID);
			return;
		}

		auto* net = ServiceLocator::TryGet<Network>();
		uint64_t localNetID = m_LocalNetworkID;
		if (localNetID == 0 && net)
		{
			localNetID = net->GetLocalNetworkID();
			if (localNetID != 0)
			{
				m_LocalNetworkID = localNetID;
			}
		}

		if (localNetID != 0 && msg->NetworkID == localNetID)
		{
			if (avatar.HasComponent<TransformComponent>())
			{
				auto& transform = avatar.GetComponent<TransformComponent>();
				TransformSystem::SetTranslation(transform, FindSpawnPosition(scene->GetRegistry()));
			}
		}

		auto& netID = avatar.AddOrReplaceComponent<NetworkIdentityComponent>();
		netID.NetworkID = msg->NetworkID;
		netID.PrefabPath = msg->PrefabPath;
		if (localNetID != 0)
		{
			netID.IsOwner = (msg->NetworkID == localNetID);
		}
		else
		{
			netID.IsOwner = false;
			CH_CORE_WARN("Network: EntitySpawn for netID={} arrived before PlayerAssign — IsOwner deferred.",
						 msg->NetworkID);
		}

		// Mark remote entities as network-driven so physics doesn't fight interpolation.
		if (!netID.IsOwner)
		{
			if (avatar.HasComponent<RigidBodyComponent>())
			{
				avatar.GetComponent<RigidBodyComponent>().IsNetworkDriven = true;
			}
		}

		m_NetworkIDToEntity[msg->NetworkID] = avatar.GetUUID();
		CH_CORE_INFO("Network: Spawned replicated entity (netID={}, owner={}).", msg->NetworkID, netID.IsOwner);
	}

	void NetworkSystem::ProcessEntityDestroyMessage(EntityDestroyMessage* msg, Scene* scene)
	{
		if (!msg || !scene)
		{
			return;
		}

		auto it = m_NetworkIDToEntity.find(msg->NetworkID);
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
		m_PendingStates.erase(msg->NetworkID);
		CH_CORE_INFO("Network: Destroyed replicated entity (netID={}).", msg->NetworkID);
	}

	void NetworkSystem::ProcessPlayerAssignMessage(PlayerAssignMessage* msg)
	{
		if (!msg)
		{
			return;
		}

		m_LocalNetworkID = msg->NetworkID;

		if (auto* net = ServiceLocator::TryGet<Network>())
		{
			net->SetLocalNetworkID(msg->NetworkID);
		}

		auto it = m_NetworkIDToEntity.find(msg->NetworkID);
		if (it != m_NetworkIDToEntity.end())
		{
			if (auto* scene = m_ReplicationScene)
			{
				Entity entity = scene->GetEntityByUUID(it->second);
				if (entity && entity.HasComponent<NetworkIdentityComponent>())
				{
					entity.GetComponent<NetworkIdentityComponent>().IsOwner = true;
					if (entity.HasComponent<RigidBodyComponent>())
					{
						entity.GetComponent<RigidBodyComponent>().IsNetworkDriven = false;
					}
				}
			}
		}

		CH_CORE_INFO("Network: Assigned local network ID {}.", msg->NetworkID);
	}

	// ---- Incoming message processing (host side) ----

	void NetworkSystem::ProcessInputStateMessage(InputStateMessage* msg, int clientIndex)
	{
		if (!msg)
		{
			return;
		}

		auto it = m_PeerToNetworkID.find(clientIndex);
		uint64_t networkID = (it != m_PeerToNetworkID.end()) ? it->second : 0;
		if (networkID == 0)
		{
			if (auto* net = ServiceLocator::TryGet<Network>())
			{
				networkID = net->GetNetworkIDForConnection(clientIndex);
				if (networkID != 0)
				{
					m_PeerToNetworkID[clientIndex] = networkID;
				}
			}
		}

		ProcessedInput input;
		input.NetworkID = networkID;
		input.MoveX = msg->MoveX;
		input.MoveZ = msg->MoveZ;
		input.ActionFlags = msg->ActionFlags;
		input.MouseX = msg->MouseX;
		input.MouseY = msg->MouseY;

		m_PendingInputs.push_back(input);
	}

	void NetworkSystem::ProcessPlayerInfoMessage(PlayerInfoMessage* msg, int clientIndex)
	{
		if (!msg)
		{
			return;
		}

		auto* net = ServiceLocator::TryGet<Network>();
		if (!net)
		{
			return;
		}

		const uint64_t networkID = net->GetNetworkIDForConnection(clientIndex);
		if (networkID == 0)
		{
			CH_CORE_WARN("Network: PlayerInfo from client {} arrived before PlayerAssign — deferring.", clientIndex);
			m_PendingPlayerInfo[clientIndex] = {std::string(msg->Name), msg->SkinIndex};
			return;
		}

		auto& playerList = net->GetPlayerListMutable();
		for (auto& p : playerList)
		{
			if (p.NetworkID != networkID)
			{
				continue;
			}

			p.Name = msg->Name;
			p.SkinIndex = msg->SkinIndex;
			CH_CORE_INFO("Network: Updated player info: '{}' (netID={}, skin={}).", p.Name, p.NetworkID,
						 (int)p.SkinIndex);
			break;
		}

		net->BroadcastPlayerList();
	}

	void NetworkSystem::ProcessPlayerListMessage(PlayerListMessage* msg)
	{
		if (!msg)
		{
			return;
		}

		auto* net = ServiceLocator::TryGet<Network>();
		if (!net)
		{
			return;
		}

		auto& playerList = net->GetPlayerListMutable();
		playerList.clear();

		for (int i = 0; i < msg->Count && i < 64; ++i)
		{
			PlayerNetInfo info;
			info.NetworkID = msg->Entries[i].NetworkID;
			info.Name = msg->Entries[i].Name;
			info.SkinIndex = msg->Entries[i].SkinIndex;
			info.IsHost = msg->Entries[i].IsHost;
			info.Ping = 0;

			playerList.push_back(info);
		}

		CH_CORE_INFO("Network: Received player list ({} players).", msg->Count);
	}

	void NetworkSystem::ProcessChatMessageMessage(ChatMessageMessage* msg)
	{
		if (!msg)
		{
			return;
		}

		ChatMessagePacket pkt;
		pkt.SenderNetworkID = msg->SenderNetworkID;
		pkt.SenderName = msg->SenderName;
		pkt.Message = msg->Message;

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
				CH_CORE_WARN("Network: ApplyHostInputs — no entity for netID={}", input.NetworkID);
				continue;
			}

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

				float moveX = input.MoveX * speed;
				float moveZ = input.MoveZ * speed;
				rb.Velocity = glm::vec3(moveX, rb.Velocity.y, moveZ);

				// Rotate remote avatar to face movement direction
				if (auto* tc = reg.try_get<TransformComponent>(targetEntity))
				{
					if (std::abs(moveX) > 0.001f || std::abs(moveZ) > 0.001f)
					{
						float yaw = std::atan2(moveX, moveZ);
						TransformSystem::SetRotation(*tc, glm::vec3(0.0f, yaw, 0.0f));
					}
				}

				if ((input.ActionFlags & InputAction_Jump) && rb.IsGrounded)
				{
					rb.Velocity.y = player.JumpForce;
					rb.VelocityForced = true;
				}

				m_LastActionFlags[input.NetworkID] = input.ActionFlags;
			}
			else
			{
				CH_CORE_WARN("Network: ApplyHostInputs — entity netID={} missing PlayerComponent/RigidBodyComponent",
							 input.NetworkID);
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

		InputStateMessage msg;
		msg.Tick = m_ClientTick++;
		msg.DeltaTime = dt;

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

		float len = std::sqrt(moveX * moveX + moveZ * moveZ);
		if (len > 0.001f)
		{
			moveX /= len;
			moveZ /= len;
		}

		msg.MoveX = moveX;
		msg.MoveZ = moveZ;

		uint8_t flags = 0;
		if (Core::Input::IsKeyPressed(KeyCode::Space))
		{
			flags |= InputAction_Jump;
		}
		if (Core::Input::IsKeyDown(KeyCode::LeftShift))
		{
			flags |= InputAction_Sprint;
		}
		msg.ActionFlags = flags;

		glm::vec2 mouseDelta = Core::Input::GetMouseDelta();
		msg.MouseX = mouseDelta.x;
		msg.MouseY = mouseDelta.y;

		ByteWriter w;
		msg.Encode(w);
		net->SendToServer(MessageType_InputState, w.Data().data(), w.Data().size(), true);
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
				// No pending state: for non-owned entities, keep NetworkDriven so
				// physics doesn't interfere; for owned, let physics run.
				if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
				{
					rb->IsNetworkDriven = !netID.IsOwner;
				}
				continue;
			}

			auto& target = it->second;
			bool changed = false;

			if (netID.IsOwner)
			{
				// Owned entity — physics runs locally, network only snap-corrects.
				if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
				{
					rb->IsNetworkDriven = false;
				}

				float dist = glm::length(TransformSystem::GetTranslation(transform) - target.TargetPosition);

				if (dist > NetworkSystem::kMaxCorrectionDistance)
				{
					TransformSystem::SetTranslation(transform, target.TargetPosition);
					TransformSystem::SetRotationQuat(transform, glm::normalize(target.TargetRotation));

					if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
					{
						rb->Velocity = target.TargetVelocity;
					}
					changed = true;
				}
			}
			else
			{
				// Remote entity — fully driven by network interpolation.
				if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
				{
					rb->IsNetworkDriven = true;
				}

				TransformSystem::SetTranslation(
					transform, glm::mix(TransformSystem::GetTranslation(transform), target.TargetPosition, t));

				// Use the local RotationQuat for slerp (not WorldTransform which may be stale).
				glm::quat currentQuat = transform.RotationQuat;
				glm::quat targetQuat = glm::normalize(target.TargetRotation);
				glm::quat blended = glm::slerp(currentQuat, targetQuat, t);
				TransformSystem::SetRotationQuat(transform, blended);

				if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
				{
					rb->Velocity = target.TargetVelocity;
					rb->IsGrounded = target.IsGrounded;
				}
				if (auto* netID = reg.try_get<NetworkIdentityComponent>(entity))
				{
					netID->RemoteActionFlags = target.ActionFlags;
				}
				changed = true;
			}

			if (changed)
			{
				glm::mat4 localMatrix = TransformSystem::ComputeLocalMatrix(transform);
				if (reg.all_of<HierarchyComponent>(entity))
				{
					auto& hc = reg.get<HierarchyComponent>(entity);
					if (hc.Parent != entt::null && reg.valid(hc.Parent) && reg.all_of<TransformComponent>(hc.Parent))
					{
						transform.WorldTransform = reg.get<TransformComponent>(hc.Parent).WorldTransform * localMatrix;
					}
					else
					{
						transform.WorldTransform = localMatrix;
					}
				}
				else
				{
					transform.WorldTransform = localMatrix;
				}
				transform.InverseWorldTransform = glm::inverse(transform.WorldTransform);
				transform.TransformChanged = false;
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
			bool IsGrounded;
			uint8_t ActionFlags;
		};

		std::vector<EntityNetState> states;
		auto view = reg.view<NetworkIdentityComponent, TransformComponent>();
		for (auto entity : view)
		{
			auto& netID = view.get<NetworkIdentityComponent>(entity);
			auto& transform = view.get<TransformComponent>(entity);

			EntityNetState s;
			s.NetworkID = netID.NetworkID;
			s.Position = TransformSystem::GetTranslation(transform);
			s.Rotation = glm::quat_cast(transform.WorldTransform);
			if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
			{
				s.Velocity = rb->Velocity;
				s.IsGrounded = rb->IsGrounded;
			}
			else
			{
				s.Velocity = {0, 0, 0};
				s.IsGrounded = false;
			}
			auto flagIt = m_LastActionFlags.find(netID.NetworkID);
			s.ActionFlags = (flagIt != m_LastActionFlags.end()) ? flagIt->second : 0;
			states.push_back(s);
		}

		if (!states.empty())
		{
			CH_CORE_INFO("Network: Broadcasting WorldState for {} entities (tick={}).", states.size(), m_HostTick);
		}

		for (auto& s : states)
		{
			net->BroadcastPacket(MessageType_WorldState, true, [this, &s](ByteWriter& bw) {
				WorldStateMessage msg;
				msg.Tick = m_HostTick++;
				msg.NetworkID = s.NetworkID;
				msg.Position[0] = s.Position.x;
				msg.Position[1] = s.Position.y;
				msg.Position[2] = s.Position.z;
				msg.Rotation[0] = s.Rotation.w;
				msg.Rotation[1] = s.Rotation.x;
				msg.Rotation[2] = s.Rotation.y;
				msg.Rotation[3] = s.Rotation.z;
				msg.Velocity[0] = s.Velocity.x;
				msg.Velocity[1] = s.Velocity.y;
				msg.Velocity[2] = s.Velocity.z;
				msg.IsGrounded = s.IsGrounded ? 1 : 0;
				msg.ActionFlags = s.ActionFlags;
				msg.Encode(bw);
			});
		}
	}

	// ---- Host-side: broadcast an entity spawn/destroy ----

	void NetworkSystem::SendEntitySpawn(Network* net, uint64_t networkID, const std::string& prefabPath,
										int clientIndex)
	{
		if (clientIndex == kInvalidPeerHandle)
		{
			net->BroadcastPacket(MessageType_EntitySpawn, true, [networkID, &prefabPath](ByteWriter& bw) {
				EntitySpawnMessage msg;
				msg.NetworkID = networkID;
				std::strncpy(msg.PrefabPath, prefabPath.c_str(), sizeof(msg.PrefabPath) - 1);
				msg.PrefabPath[sizeof(msg.PrefabPath) - 1] = '\0';
				msg.Encode(bw);
			});
		}
		else
		{
			EntitySpawnMessage msg;
			msg.NetworkID = networkID;
			std::strncpy(msg.PrefabPath, prefabPath.c_str(), sizeof(msg.PrefabPath) - 1);
			msg.PrefabPath[sizeof(msg.PrefabPath) - 1] = '\0';
			ByteWriter w;
			msg.Encode(w);
			net->SendPacket(clientIndex, MessageType_EntitySpawn, w.Data().data(), w.Data().size(), true);
		}
	}

	void NetworkSystem::SendEntityDestroy(Network* net, uint64_t networkID)
	{
		net->BroadcastPacket(MessageType_EntityDestroy, true, [networkID](ByteWriter& bw) {
			EntityDestroyMessage msg;
			msg.NetworkID = networkID;
			msg.Encode(bw);
		});
	}

	void NetworkSystem::ResyncClientEntities(int clientIndex, Scene* scene)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || !scene)
		{
			return;
		}

		uint64_t clientNetID = 0;
		auto it = m_PeerToNetworkID.find(clientIndex);
		if (it != m_PeerToNetworkID.end())
		{
			clientNetID = it->second;
		}
		else
		{
			clientNetID = net->GetNetworkIDForConnection(clientIndex);
		}

		int count = 0;
		if (clientNetID != 0)
		{
			PlayerAssignMessage assignMsg;
			assignMsg.NetworkID = clientNetID;
			ByteWriter w;
			assignMsg.Encode(w);
			net->SendPacket(clientIndex, MessageType_PlayerAssign, w.Data().data(), w.Data().size(), true);
		}

		entt::registry& reg = scene->GetRegistry();
		auto view = reg.view<NetworkIdentityComponent>();
		for (auto entity : view)
		{
			auto& netID = view.get<NetworkIdentityComponent>(entity);
			if (netID.NetworkID == 0)
			{
				continue;
			}
			std::string prefabPath = netID.PrefabPath.empty() ? m_PlayerPrefab : netID.PrefabPath;
			SendEntitySpawn(net, netID.NetworkID, prefabPath, clientIndex);
			count++;
		}

		CH_CORE_INFO("Network: Resynced {} entities for client {} (netID={}).", count, clientIndex, clientNetID);
	}

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
				return;
			}
		}

		auto players = reg.view<PlayerComponent>();
		for (auto entity : players)
		{
			if (reg.all_of<NetworkIdentityComponent>(entity))
			{
				continue;
			}

			auto& netID = reg.emplace<NetworkIdentityComponent>(entity);
			netID.NetworkID = HostAvatarNetworkID;
			netID.IsOwner = true;
			netID.PrefabPath = m_PlayerPrefab;

			if (reg.all_of<TransformComponent>(entity))
			{
				auto& transform = reg.get<TransformComponent>(entity);
				TransformSystem::SetTranslation(transform, FindSpawnPosition(reg));
			}

			CH_CORE_INFO("Network: Tagged host player with netID={}.", HostAvatarNetworkID);
			return;
		}

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
					TransformSystem::SetTranslation(transform, FindSpawnPosition(reg));
				}

				auto& netID = hostAvatar.AddOrReplaceComponent<NetworkIdentityComponent>();
				netID.NetworkID = HostAvatarNetworkID;
				netID.IsOwner = true;
				netID.PrefabPath = m_PlayerPrefab;

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
			if (!m_PrefabWarnedOnce && net->GetClientCount() > 0)
			{
				m_PrefabWarnedOnce = true;
				CH_CORE_ERROR("Network: No player prefab configured — clients will connect without an avatar. "
							  "Call NetworkSystem::SetPlayerPrefab().");
			}
			return;
		}

		for (int clientIndex = 0; clientIndex < net->GetMaxClients(); ++clientIndex)
		{
			if (!net->IsClientConnected(clientIndex))
			{
				continue;
			}

			if (m_PeerToAvatar.find(clientIndex) != m_PeerToAvatar.end())
			{
				continue;
			}

			uint64_t networkID = net->GetNetworkIDForConnection(clientIndex);
			if (networkID == 0)
			{
				continue;
			}
			m_PeerToNetworkID[clientIndex] = networkID;

			std::string path = m_PlayerPrefab;
			if (auto* am = ServiceLocator::TryGet<AssetManager>())
			{
				path = am->ResolvePath(m_PlayerPrefab);
			}

			Entity avatar = PrefabSerializer::Deserialize(scene, path);
			if (!avatar)
			{
				CH_CORE_ERROR("Network: Failed to spawn player prefab '{}' for client {}.", m_PlayerPrefab,
							  clientIndex);
				m_PeerToAvatar[clientIndex] = UUID(0);
				continue;
			}

			if (avatar.HasComponent<TransformComponent>())
			{
				auto& transform = avatar.GetComponent<TransformComponent>();
				glm::vec3 spawnPos = FindSpawnPosition(scene->GetRegistry());
				spawnPos.x += static_cast<float>(networkID - 1) * 1.5f;
				TransformSystem::SetTranslation(transform, spawnPos);
			}

			auto& netID = avatar.AddOrReplaceComponent<NetworkIdentityComponent>();
			netID.NetworkID = networkID;
			netID.IsOwner = false;
			netID.PrefabPath = m_PlayerPrefab;

			m_PeerToAvatar[clientIndex] = avatar.GetUUID();
			CH_CORE_INFO("Network: Spawned avatar (netID={}) for client {}.", networkID, clientIndex);

			for (const auto& [existingClient, uuid] : m_PeerToAvatar)
			{
				if (existingClient == clientIndex || uuid == UUID(0))
				{
					continue;
				}
				auto netIdIt = m_PeerToNetworkID.find(existingClient);
				if (netIdIt != m_PeerToNetworkID.end())
				{
					SendEntitySpawn(net, netIdIt->second, m_PlayerPrefab, clientIndex);
				}
			}
			SendEntitySpawn(net, HostAvatarNetworkID, m_PlayerPrefab, clientIndex);
			SendEntitySpawn(net, networkID, m_PlayerPrefab, kInvalidPeerHandle);
		}

		for (auto it = m_PeerToAvatar.begin(); it != m_PeerToAvatar.end();)
		{
			int clientIndex = it->first;
			if (net->IsClientConnected(clientIndex))
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

			auto idIt = m_PeerToNetworkID.find(clientIndex);
			if (idIt != m_PeerToNetworkID.end())
			{
				SendEntityDestroy(net, idIt->second);
			}

			UnregisterPeer(clientIndex);
			CH_CORE_INFO("Network: Despawned avatar for client {}.", clientIndex);
			it = m_PeerToAvatar.erase(it);
		}
	}

	void NetworkSystem::InstallPacketCallback()
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net)
		{
			return;
		}
		// Re-install whenever the role has changed (including Offline→Host on each new session)
		if (m_CallbackRole == net->GetRole() && net->GetRole() != Role::Offline)
		{
			return;
		}

		m_CallbackRole = net->GetRole();
		net->SetPacketCallback([this, net](int clientIndex, MessageType type, const uint8_t* data, size_t len) {
			ByteReader r(data, len);

			if (net->GetRole() == Role::Host)
			{
				switch (type)
				{
				case MessageType_InputState: {
					InputStateMessage msg;
					if (msg.Decode(r))
					{
						ProcessInputStateMessage(&msg, clientIndex);
					}
					break;
				}
				case MessageType_PlayerInfo: {
					PlayerInfoMessage msg;
					if (msg.Decode(r))
					{
						ProcessPlayerInfoMessage(&msg, clientIndex);
					}
					break;
				}
				case MessageType_ChatMessage: {
					ChatMessageMessage msg;
					if (msg.Decode(r))
					{
						ProcessChatMessageMessage(&msg);
						ChatMessagePacket pkt;
						pkt.SenderNetworkID = msg.SenderNetworkID;
						pkt.SenderName = msg.SenderName;
						pkt.Message = msg.Message;
						net->StorePendingChatMessage(pkt);
						if (net->IsHost())
						{
							net->BroadcastPacket(MessageType_ChatMessage, true,
												 [&msg](ByteWriter& bw) { msg.Encode(bw); });
						}
					}
					break;
				}
				case MessageType_SceneLoaded: {
					CH_CORE_INFO("Network: Client {} loaded scene — resyncing entities.", clientIndex);
					ResyncClientEntities(clientIndex, m_ReplicationScene);
					break;
				}
				default:
					break;
				}
			}
			else if (net->GetRole() == Role::Client)
			{
				switch (type)
				{
				case MessageType_WorldState: {
					WorldStateMessage msg;
					if (msg.Decode(r))
					{
						ProcessWorldStateMessage(&msg);
					}
					break;
				}
				case MessageType_SceneChange: {
					SceneChangeMessage msg;
					if (msg.Decode(r))
					{
						ProcessSceneChangeMessage(&msg);
					}
					break;
				}
				case MessageType_PlayerList: {
					PlayerListMessage msg;
					if (msg.Decode(r))
					{
						ProcessPlayerListMessage(&msg);
					}
					break;
				}
				case MessageType_PlayerAssign: {
					PlayerAssignMessage msg;
					if (msg.Decode(r))
					{
						ProcessPlayerAssignMessage(&msg);
					}
					break;
				}
				case MessageType_EntitySpawn: {
					EntitySpawnMessage msg;
					if (msg.Decode(r))
					{
						ProcessEntitySpawnMessage(&msg, m_ReplicationScene);
					}
					break;
				}
				case MessageType_EntityDestroy: {
					EntityDestroyMessage msg;
					if (msg.Decode(r))
					{
						ProcessEntityDestroyMessage(&msg, m_ReplicationScene);
					}
					break;
				}
				case MessageType_ChatMessage: {
					ChatMessageMessage msg;
					if (msg.Decode(r))
					{
						ProcessChatMessageMessage(&msg);
						ChatMessagePacket pkt;
						pkt.SenderNetworkID = msg.SenderNetworkID;
						pkt.SenderName = msg.SenderName;
						pkt.Message = msg.Message;
						net->StorePendingChatMessage(pkt);
					}
					break;
				}
				default:
					break;
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

	// ---- Per-session reset ----

	void NetworkSystem::Reset()
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (net && net->IsClient() && net->GetLocalNetworkID() != 0)
		{
			m_LocalNetworkID = net->GetLocalNetworkID();
		}
		else
		{
			m_LocalNetworkID = 0;
		}

		m_ClientTick = 0;
		m_CallbackRole = Role::Offline; // forces callback re-install on next session
		m_PeerToNetworkID.clear();
		m_PeerToAvatar.clear();
		m_PendingStates.clear();
		m_PendingInputs.clear();
		m_PendingChatMessages.clear();
		m_PendingPlayerInfo.clear();
		m_NetworkIDToEntity.clear();
		m_LastActionFlags.clear();
		m_ReplicationScene = nullptr;
		m_PrefabWarnedOnce = false;
		m_SceneLoadedPending = false;

		if (net)
		{
			net->ClearPendingSceneChange();
		}

		CH_CORE_INFO("NetworkSystem: session state reset (localNetID={}).", m_LocalNetworkID);
	}

	// ---- Main update ----

	void NetworkSystem::CheckAndPropagateSceneChange(Scene* scene)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || !scene)
		{
			return;
		}
		if (!net->HasPendingSceneChange())
		{
			return;
		}

		std::string path = net->GetPendingSceneChange();
		CH_CORE_INFO("CheckAndPropagateSceneChange: propagating '{}' to Scene", path);
		scene->SetPendingScenePath(path);
		net->ClearPendingSceneChange();

		SceneChangeRequestEvent e(path);
		Application::Get().OnEvent(e);
	}

	void NetworkSystem::PollNetwork(Scene* scene, Timestep ts)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || net->GetRole() == Role::Offline || !scene)
		{
			return;
		}

		entt::registry& reg = scene->GetRegistry();

		// Scene pointer changed → new Play Mode session, reset lookup tables
		if (m_ReplicationScene != scene)
		{
			m_ReplicationScene = scene;
			m_NetworkIDToEntity.clear();
			m_PeerToAvatar.clear();
			m_PendingStates.clear();
			if (net->IsClient())
			{
				m_SceneLoadedPending = true;
			}
		}

		InstallPacketCallback();

		if (m_SceneLoadedPending && net->IsClient())
		{
			m_SceneLoadedPending = false;
			SceneLoadedMessage msg;
			ByteWriter w;
			msg.Encode(w);
			net->SendToServer(MessageType_SceneLoaded, w.Data().data(), w.Data().size(), true);
			CH_CORE_INFO("Network: Sent SceneLoaded to host.");
		}

		EnsureLocalIdentity(scene); // BUG4 fix: must run before scripts

		float dt = static_cast<float>(ts);

		net->Update(dt);

		// Check for pending scene change AFTER processing ENet packets.
		// This avoids the C# polling timing issue where OnUpdate runs before
		// net->Update processes incoming SceneChangeMessage packets.
		if (net->IsClient())
		{
			CheckAndPropagateSceneChange(scene);
		}

		if (net->IsHost())
		{
			// Flush any deferred PlayerInfoMessages (race: PlayerInfo arrived before PlayerAssign).
			for (auto it = m_PendingPlayerInfo.begin(); it != m_PendingPlayerInfo.end();)
			{
				int clientIndex = it->first;
				uint64_t netId = net->GetNetworkIDForConnection(clientIndex);
				if (netId != 0)
				{
					const auto& info = it->second;
					PlayerInfoMessage deferredMsg;
					std::strncpy(deferredMsg.Name, info.first.c_str(), sizeof(deferredMsg.Name) - 1);
					deferredMsg.Name[sizeof(deferredMsg.Name) - 1] = '\0';
					deferredMsg.SkinIndex = info.second;
					ProcessPlayerInfoMessage(&deferredMsg, clientIndex);
					it = m_PendingPlayerInfo.erase(it);
				}
				else
				{
					++it;
				}
			}

			EnsureHostIdentity(scene);
			SyncPeerAvatars(scene, net);
		}
	}

	void NetworkSystem::FinalizeFrame(Scene* scene, Timestep ts)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || net->GetRole() == Role::Offline || !scene)
		{
			return;
		}

		entt::registry& reg = scene->GetRegistry();

		if (net->IsHost())
		{
			BroadcastWorldState(reg);
		}
		else if (net->IsClient())
		{
			float dt = static_cast<float>(ts);
			CollectAndSendInput(net, dt);
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
