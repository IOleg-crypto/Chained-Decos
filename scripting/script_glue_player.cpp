#include "script_glue_entity.h"
#include "engine/scene/components.h"
#include "engine/scene/entity.h"

namespace Chained
{

	// ── PlayerComponent ──────────────────────────────────────────────────

	float PlayerComponent_GetMovementSpeed(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<Chained::PlayerComponent>()
				   ? entity.GetComponent<Chained::PlayerComponent>().MovementSpeed
				   : 0.0f;
	}
	void PlayerComponent_SetMovementSpeed(uint64_t entityID, float value)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<Chained::PlayerComponent>())
		{
			entity.GetComponent<Chained::PlayerComponent>().MovementSpeed = value;
		}
	}
	float PlayerComponent_GetJumpForce(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<Chained::PlayerComponent>()
				   ? entity.GetComponent<Chained::PlayerComponent>().JumpForce
				   : 0.0f;
	}
	void PlayerComponent_SetJumpForce(uint64_t entityID, float value)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<Chained::PlayerComponent>())
		{
			entity.GetComponent<Chained::PlayerComponent>().JumpForce = value;
		}
	}
	float PlayerComponent_GetLookSensitivity(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<Chained::PlayerComponent>()
				   ? entity.GetComponent<Chained::PlayerComponent>().LookSensitivity
				   : 0.0f;
	}
	void PlayerComponent_SetLookSensitivity(uint64_t entityID, float value)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<Chained::PlayerComponent>())
		{
			entity.GetComponent<Chained::PlayerComponent>().LookSensitivity = value;
		}
	}

	// ── SpawnComponent ───────────────────────────────────────────────────

	uint8_t SpawnComponent_GetIsActive(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<Chained::SpawnComponent>()
				   ? entity.GetComponent<Chained::SpawnComponent>().IsActive
				   : false;
	}
	void SpawnComponent_SetIsActive(uint64_t entityID, uint8_t value)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<Chained::SpawnComponent>())
		{
			entity.GetComponent<Chained::SpawnComponent>().IsActive = value;
		}
	}
	uint8_t SpawnComponent_IsCheckpoint(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<Chained::SpawnComponent>()
				   ? entity.GetComponent<Chained::SpawnComponent>().IsCheckpoint
				   : false;
	}
	void SpawnComponent_SetIsCheckpoint(uint64_t entityID, uint8_t value)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<Chained::SpawnComponent>())
		{
			entity.GetComponent<Chained::SpawnComponent>().IsCheckpoint = value;
		}
	}
	void SpawnComponent_GetSpawnPoint(uint64_t entityID, glm::vec3* outPoint)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<Chained::SpawnComponent>() && outPoint)
		{
			*outPoint = entity.GetComponent<Chained::SpawnComponent>().SpawnPoint;
		}
		else if (outPoint)
		{
			*outPoint = {};
		}
	}
	void SpawnComponent_SetSpawnPoint(uint64_t entityID, glm::vec3* inPoint)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<Chained::SpawnComponent>() && inPoint)
		{
			entity.GetComponent<Chained::SpawnComponent>().SpawnPoint = *inPoint;
		}
	}
	uint8_t SpawnComponent_GetRenderSpawnZoneInScene(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<Chained::SpawnComponent>()
				   ? entity.GetComponent<Chained::SpawnComponent>().RenderSpawnZoneInScene
				   : false;
	}
	void SpawnComponent_GetZoneSize(uint64_t entityID, glm::vec3* outSize)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<Chained::SpawnComponent>() && outSize)
		{
			*outSize = entity.GetComponent<Chained::SpawnComponent>().ZoneSize;
		}
		else if (outSize)
		{
			*outSize = {};
		}
	}

} // namespace Chained
