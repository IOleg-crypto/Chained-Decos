#include "script_glue_entity.h"
#include "engine/audio/audio.h"
#include "engine/core/service_locator.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/entity.h"

namespace Chained
{

	// ── RigidBody ────────────────────────────────────────────────────────

	void RigidBody_GetVelocity(uint64_t entityID, glm::vec3* outVelocity)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<RigidBodyComponent>() && outVelocity)
		{
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			if (rb.Handle != kInvalidPhysicsBody)
			{
				auto* physics = ServiceLocator::TryGet<Physics>();
				if (physics && physics->GetWorld())
				{
					*outVelocity = physics->GetWorld()->GetVelocity(rb.Handle);
					return;
				}
			}
			*outVelocity = rb.Velocity;
		}
		else if (outVelocity)
		{
			*outVelocity = {};
		}
	}
	void RigidBody_SetVelocity(uint64_t entityID, glm::vec3* inVelocity)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<RigidBodyComponent>() && inVelocity)
		{
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			rb.Velocity = *inVelocity;
			if (rb.Handle != kInvalidPhysicsBody)
			{
				auto* physics = ServiceLocator::TryGet<Physics>();
				if (physics && physics->GetWorld())
				{
					glm::vec3 toSet = *inVelocity;
					if (rb.Type == RigidBodyComponent::BodyType::Dynamic)
					{
						constexpr float kJumpImpulseThreshold = 0.5f;
						if (toSet.y <= kJumpImpulseThreshold)
						{
							toSet.y = physics->GetWorld()->GetVelocity(rb.Handle).y;
						}
					}
					physics->GetWorld()->SetVelocity(rb.Handle, toSet);
				}
			}
		}
	}
	void RigidBody_ForceSetVelocity(uint64_t entityID, glm::vec3* inVelocity)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<RigidBodyComponent>() && inVelocity)
		{
			auto& rb = entity.GetComponent<RigidBodyComponent>();
			rb.Velocity = *inVelocity;
			rb.VelocityForced = true;
			if (rb.Handle != kInvalidPhysicsBody)
			{
				auto* physics = ServiceLocator::TryGet<Physics>();
				if (physics)
				{
					physics->ForceSetVelocity(rb.Handle, *inVelocity);
				}
			}
		}
	}
	bool RigidBody_IsGrounded(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (!entity || !entity.HasComponent<RigidBodyComponent>())
		{
			return false;
		}

		auto& rb = entity.GetComponent<RigidBodyComponent>();
		return rb.IsGrounded;
	}
	uint32_t RigidBody_IsKinematic(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return (entity && entity.HasComponent<RigidBodyComponent>() &&
				entity.GetComponent<RigidBodyComponent>().Type == RigidBodyComponent::BodyType::Kinematic)
				   ? 1u
				   : 0u;
	}
	void RigidBody_SetKinematic(uint64_t entityID, bool isKinematic)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<RigidBodyComponent>())
		{
			entity.GetComponent<RigidBodyComponent>().Type =
				isKinematic ? RigidBodyComponent::BodyType::Kinematic : RigidBodyComponent::BodyType::Dynamic;
		}
	}

	// ── Audio (on entity) ────────────────────────────────────────────────

	void AudioComponent_Play(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<AudioComponent>())
		{
			auto& audio = entity.GetComponent<AudioComponent>();
			if (audio.SoundHandle != 0)
			{
				auto* audioService = ServiceLocator::TryGet<Audio>();
				if (!audioService)
				{
					return;
				}

				glm::vec3 worldPos = {0.0f, 0.0f, 0.0f};
				if (entity.HasComponent<TransformComponent>())
				{
					worldPos = glm::vec3(entity.GetComponent<TransformComponent>().WorldTransform[3]);
				}

				audioService->SetInstancePosition(audio.SoundHandle, worldPos);
				audioService->Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized,
								   worldPos);
				audio.IsPlaying = true;
			}
		}
	}
	void AudioComponent_Stop(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<AudioComponent>())
		{
			auto& audio = entity.GetComponent<AudioComponent>();
			if (audio.SoundHandle != 0 && audio.IsPlaying)
			{
				auto* audioService = ServiceLocator::TryGet<Audio>();
				if (!audioService)
				{
					return;
				}
				audioService->Stop(audio.SoundHandle);
				audio.IsPlaying = false;
			}
		}
	}

} // namespace Chained
