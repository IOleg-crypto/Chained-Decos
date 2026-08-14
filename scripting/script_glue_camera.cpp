#include "script_glue_camera.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include "engine/physics/physics.h"
#include "engine/physics/raycast_result.h"
#include "engine/scene/components.h"

namespace Chained
{
	void Camera_GetForward(uint64_t entityID, glm::vec3* outForward)
	{
		if (!outForward)
		{
			return;
		}
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<TransformComponent>())
		{
			auto& tc = entity.GetComponent<TransformComponent>();
			glm::quat rotation = tc.RotationQuat;
			*outForward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
		}
		else
		{
			CH_CORE_WARN("[Diag Camera_GetForward] entityID={} NOT FOUND or no TransformComponent!", entityID);
			*outForward = {};
		}
	}
	void Camera_GetRight(uint64_t entityID, glm::vec3* outRight)
	{
		if (!outRight)
		{
			return;
		}
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<TransformComponent>())
		{
			auto& tc = entity.GetComponent<TransformComponent>();
			glm::quat rotation = tc.RotationQuat;
			*outRight = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
		}
		else
		{
			*outRight = {};
		}
	}
	void Camera_SetOrbit(uint64_t entityID, float yaw, float pitch, float distance)
	{
		Entity entity = GetEntity(entityID);
		Scene* scene = GetActiveScene();

		if (entity && entity.HasComponent<CameraComponent>() && entity.HasComponent<TransformComponent>() && scene)
		{
			auto& camera = entity.GetComponent<CameraComponent>();

			// 1. Спочатку обмежуємо pitch, щоб збережені дані відповідали реальності
			pitch = glm::clamp(pitch, 5.0f, 75.0f);

			camera.OrbitYaw = yaw;
			camera.OrbitPitch = pitch;
			camera.OrbitDistance = distance;

			auto& tc = entity.GetComponent<TransformComponent>();
			Entity target;

			// Пошук гравця (мережевий або одиночний)
			auto netView = scene->GetRegistry().view<NetworkIdentityComponent>();
			for (auto e : netView)
			{
				const auto& netID = netView.get<NetworkIdentityComponent>(e);
				if (netID.IsOwner)
				{
					target = Entity(e, scene->GetRegistryPtr());
					break;
				}
			}

			if (!target)
			{
				target = scene->FindEntityByTag(camera.TargetEntityTag);
			}

			glm::vec3 targetPos = glm::vec3(0.0f);
			if (target && target.HasComponent<TransformComponent>())
			{
				const auto& targetTC = target.GetComponent<TransformComponent>();
				targetPos = glm::vec3(targetTC.WorldTransform[3]);
			}

			// 2. Встановлюємо Pivot рівно на висоту очей/грудей (+1.8m)
			glm::vec3 pivot = targetPos + glm::vec3(0.0f, 1.8f, 0.0f);

			// 3. Правильне обчислення кватерніона обертання
			float yawRad = glm::radians(yaw);
			float pitchRad = glm::radians(pitch);

			glm::quat yawQuat = glm::angleAxis(yawRad, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::quat pitchQuat = glm::angleAxis(-pitchRad, glm::vec3(1.0f, 0.0f, 0.0f));
			glm::quat rotation = yawQuat * pitchQuat;

			glm::vec3 offset = rotation * glm::vec3(0.0f, 0.0f, distance);
			glm::vec3 newPos = pivot + offset;

			// // 4. Безпечний Raycast від перешкод
			// if (distance > 0.6f)
			// {
			//     if (auto* physics = ServiceLocator::TryGet<Physics>())
			//     {
			//         glm::vec3 rayDir = glm::normalize(offset);
			//         float startOffset = 0.6f;

			//         Ray ray;
			//         ray.position = pivot + rayDir * startOffset;
			//         ray.direction = rayDir;

			//         RaycastResult hit = physics->Raycast(ray);
			//         float maxRayDist = distance - startOffset;
			//         if (hit.Hit && hit.Distance < maxRayDist)
			//         {
			//             float safeDist = startOffset + glm::max(hit.Distance - 0.2f, 0.0f);
			//             newPos = pivot + rayDir * safeDist;
			//         }
			//     }
			// }

			ComponentUtils::SetTranslation(tc, newPos);
			ComponentUtils::SetRotationQuat(tc, rotation);
		}
	}
	void Camera_GetOrbit(uint64_t entityID, float* yaw, float* pitch, float* distance)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<CameraComponent>())
		{
			auto& camera = entity.GetComponent<CameraComponent>();
			if (yaw)
			{
				*yaw = camera.OrbitYaw;
			}
			if (pitch)
			{
				*pitch = camera.OrbitPitch;
			}
			if (distance)
			{
				*distance = camera.OrbitDistance;
			}
		}
	}
	uint8_t Camera_GetPrimary(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().Primary
																: false;
	}
	void Camera_SetPrimary(uint64_t entityID, uint8_t primary)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<CameraComponent>())
		{
			CH_CORE_TRACE("[ScriptGlue] Camera_SetPrimary: Entity={}, Primary={}", (uint32_t)entityID, primary);
			entity.GetComponent<CameraComponent>().Primary = primary;
		}
	}
	void Camera_SetIsOrbit(uint64_t entityID, uint8_t isOrbit)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<CameraComponent>())
		{
			entity.GetComponent<CameraComponent>().IsOrbitCamera = isOrbit;
		}
	}
	const Coral::UCChar* Camera_GetTargetTag(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		std::string tag = entity && entity.HasComponent<CameraComponent>()
							  ? entity.GetComponent<CameraComponent>().TargetEntityTag
							  : "";
		return GlueStringPool::ReturnString(tag);
	}
	void Camera_SetTargetTag(uint64_t entityID, const Coral::UCChar* tag)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<CameraComponent>() && tag)
		{
			entity.GetComponent<CameraComponent>().TargetEntityTag = ch_u16_to_string(tag);
		}
	}

	uint8_t Camera_GetIsOrbit(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<CameraComponent>() ? entity.GetComponent<CameraComponent>().IsOrbitCamera
																: false;
	}
} // namespace Chained
