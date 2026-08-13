#include "network_system.h"
#include "engine/scene/components/gameplay/network_identity_component.h"
#include "engine/scene/components/core/transform_component.h"
#include "engine/scene/components/gameplay/player_component.h"
#include "engine/scene/components/physics/physics_component.h"
#include "engine/scene/components/render/camera_component.h"
#include "engine/scene/components/gameplay/spawn_component.h"
#include "engine/networking/network_service.h"
#include "engine/scene/scene.h"
#include "engine/scene/prefab_serializer.h"
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

		PendingNetworkState state;
		state.NetworkID = msg->NetworkID;
		state.TargetPosition = {msg->Position[0], msg->Position[1], msg->Position[2]};
		state.TargetRotation = {msg->Rotation[0], msg->Rotation[1], msg->Rotation[2], msg->Rotation[3]};
		state.TargetVelocity = {msg->Velocity[0], msg->Velocity[1], msg->Velocity[2]};

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

		if (m_LocalNetworkID != 0 && msg->NetworkID == m_LocalNetworkID)
		{
			if (avatar.HasComponent<TransformComponent>())
			{
				auto& transform = avatar.GetComponent<TransformComponent>();
				transform.Translation = FindSpawnPosition(scene->GetRegistry());
				transform.TransformChanged = true;
			}
		}

		auto& netID = avatar.AddOrReplaceComponent<NetworkIdentityComponent>();
		netID.NetworkID = msg->NetworkID;
		if (m_LocalNetworkID != 0)
		{
			netID.IsOwner = (msg->NetworkID == m_LocalNetworkID);
		}
		else
		{
			netID.IsOwner = false;
			CH_CORE_WARN("Network: EntitySpawn for netID={} arrived before PlayerAssign — IsOwner deferred.",
						 msg->NetworkID);
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
			CH_CORE_WARN("Network: PlayerInfo from unknown client {} — ignored.", clientIndex);
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
		net->SendToServer(MessageType_InputState, w.Data().data(), w.Data().size(), false);
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

			if (netID.IsOwner)
			{
				float dist = glm::length(transform.Translation - target.TargetPosition);

				if (dist > NetworkSystem::kMaxCorrectionDistance)
				{
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

		for (auto& s : states)
		{
			net->BroadcastPacket(MessageType_WorldState, false, [this, &s](ByteWriter& bw) {
				WorldStateMessage msg;
				msg.Tick = m_ClientTick;
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

			if (reg.all_of<TransformComponent>(entity))
			{
				auto& transform = reg.get<TransformComponent>(entity);
				transform.Translation = FindSpawnPosition(reg);
				transform.TransformChanged = true;
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
			static bool warned = false;
			if (!warned && net->GetClientCount() > 0)
			{
				warned = true;
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
				transform.Translation = spawnPos;
				transform.TransformChanged = true;
			}

			auto& netID = avatar.AddOrReplaceComponent<NetworkIdentityComponent>();
			netID.NetworkID = networkID;
			netID.IsOwner = false;

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
		if (!net || m_CallbackRole == net->GetRole())
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
						if (net->IsHost())
						{
							net->BroadcastPacket(MessageType_ChatMessage, true,
												 [&msg](ByteWriter& bw) { msg.Encode(bw); });
						}
					}
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

	// ---- Main update ----

	void NetworkSystem::Update(Scene* scene, Timestep ts)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || net->GetRole() == Role::Offline || !scene)
		{
			return;
		}

		entt::registry& reg = scene->GetRegistry();

		if (m_ReplicationScene != scene)
		{
			m_ReplicationScene = scene;
			m_NetworkIDToEntity.clear();
			m_PeerToAvatar.clear();
			m_PendingStates.clear();
		}

		InstallPacketCallback();

		float dt = static_cast<float>(ts);

		net->Update(dt);

		if (net->IsHost())
		{
			EnsureHostIdentity(scene);
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
