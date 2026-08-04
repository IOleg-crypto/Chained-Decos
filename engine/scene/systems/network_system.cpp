#include "network_system.h"
#include "engine/scene/components/network_identity_component.h"
#include "engine/scene/components/transform_component.h"
#include "engine/scene/components/player_component.h"
#include "engine/scene/components/physics_component.h"
#include "engine/networking/network_service.h"
#include "engine/core/service_locator.h"
#include "engine/core/input.h"
#include "engine/core/key_codes.h"
#include "engine/core/log.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cstring>

namespace Chained
{

	// Persistent interpolation buffer for client-side state smoothing.
	static std::unordered_map<uint64_t, PendingNetworkState> s_PendingStates;

	// Accumulated inputs from clients (host-side).
	static std::vector<ProcessedInput> s_PendingInputs;

	// Client tick counter.
	static uint32_t s_ClientTick = 0;

	// Peer → NetworkID mapping (host-side).
	static std::unordered_map<ENetPeer*, uint64_t> s_PeerToNetworkID;

	// ---- Peer mapping ----

	void NetworkSystem::RegisterPeerEntity(ENetPeer* peer, uint64_t networkID)
	{
		s_PeerToNetworkID[peer] = networkID;
	}

	void NetworkSystem::UnregisterPeer(ENetPeer* peer)
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

	// ---- Incoming packet processing (host side) ----

	static void ProcessInputStatePacket(const uint8_t* data, size_t size, ENetPeer* sender)
	{
		if (size < InputStatePacket::WireSize())
		{
			return;
		}

		InputStatePacket pkt = InputStatePacket::Deserialize(data);

		// Resolve peer → NetworkID
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

				// Horizontal movement — set velocity directly, preserve Y from physics
				float moveX = input.MoveX * speed;
				float moveZ = input.MoveZ * speed;
				rb.Velocity = glm::vec3(moveX, rb.Velocity.y, moveZ);

				// Jump — requires VelocityForced to bypass Y-override in Physics::Update
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

	// ---- Main update ----

	void NetworkSystem::Update(entt::registry& reg, Timestep ts)
	{
		auto* net = ServiceLocator::TryGet<Network>();
		if (!net || net->GetRole() == Role::Offline)
		{
			return;
		}

		float dt = static_cast<float>(ts);

		// Pump ENet events (process connect/disconnect/receive)
		net->Update(dt);

		if (net->IsHost())
		{
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

} // namespace Chained
