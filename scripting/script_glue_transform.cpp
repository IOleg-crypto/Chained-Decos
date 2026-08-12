#include "script_glue_entity.h"
#include "engine/core/service_locator.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/components/core/component_utils.h"
#include "engine/scene/entity.h"

namespace Chained
{

	// ── Transform ────────────────────────────────────────────────────────

	void Transform_GetTranslation(uint64_t entityID, glm::vec3* outTranslation)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<TransformComponent>() && outTranslation)
		{
			*outTranslation = entity.GetComponent<TransformComponent>().Translation;
		}
		else if (outTranslation)
		{
			*outTranslation = {};
		}
	}
	void Transform_SetTranslation(uint64_t entityID, glm::vec3* inTranslation)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<TransformComponent>() && inTranslation)
		{
			auto& tc = entity.GetComponent<TransformComponent>();
			tc.Translation = *inTranslation;
			tc.PrevTranslation = *inTranslation;
			tc.TransformChanged = true;

			if (entity.HasComponent<RigidBodyComponent>())
			{
				auto& rb = entity.GetComponent<RigidBodyComponent>();
				if (rb.Handle != kInvalidPhysicsBody)
				{
					auto* physics = ServiceLocator::TryGet<Physics>();
					if (physics && physics->GetWorld())
					{
						physics->GetWorld()->SetTransform(rb.Handle, *inTranslation, tc.RotationQuat);
					}
				}
			}
		}
	}
	void Transform_GetRotation(uint64_t entityID, glm::vec3* outRotation)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<TransformComponent>() && outRotation)
		{
			*outRotation = entity.GetComponent<TransformComponent>().Rotation;
		}
		else if (outRotation)
		{
			*outRotation = {};
		}
	}
	void Transform_SetRotation(uint64_t entityID, glm::vec3* inRotation)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<TransformComponent>() && inRotation)
		{
			ComponentUtils::SetRotation(entity.GetComponent<TransformComponent>(), *inRotation);
		}
	}
	void Transform_GetScale(uint64_t entityID, glm::vec3* outScale)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<TransformComponent>() && outScale)
		{
			*outScale = entity.GetComponent<TransformComponent>().Scale;
		}
		else if (outScale)
		{
			*outScale = {};
		}
	}
	void Transform_SetScale(uint64_t entityID, glm::vec3* inScale)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<TransformComponent>() && inScale)
		{
			ComponentUtils::SetScale(entity.GetComponent<TransformComponent>(), *inScale);
		}
	}

	// ── Model ────────────────────────────────────────────────────────────

	const Coral::UCChar* Model_GetModelPath(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		std::string path =
			entity && entity.HasComponent<ModelComponent>() ? entity.GetComponent<ModelComponent>().ModelPath : "";
		return GlueStringPool::ReturnString(path);
	}
	void Model_SetModelPath(uint64_t entityID, const Coral::UCChar* inPath)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<ModelComponent>())
		{
			entity.GetComponent<ModelComponent>().ModelPath = ch_u16_to_string(inPath);
		}
	}

	// ── Tag ──────────────────────────────────────────────────────────────

	const Coral::UCChar* TagComponent_GetTag(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		std::string tag = entity && entity.HasComponent<TagComponent>() ? entity.GetComponent<TagComponent>().Tag : "";
		return GlueStringPool::ReturnString(tag);
	}

	// ── Shader ───────────────────────────────────────────────────────────

	void Shader_SetFloat(uint64_t entityID, const Coral::UCChar* inName, float inValue)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<ShaderComponent>())
		{
			auto& shader = entity.GetComponent<ShaderComponent>();
			std::string name = ch_u16_to_string(inName);

			auto it = std::find_if(shader.Uniforms.begin(), shader.Uniforms.end(),
								   [&](const auto& u) { return u.Name == name; });

			if (it != shader.Uniforms.end())
			{
				it->Value = inValue;
			}
			else
			{
				ShaderUniform uniform;
				uniform.Name = name;
				uniform.Value = inValue;
				shader.Uniforms.push_back(uniform);
			}
		}
	}
	void Shader_SetVec3(uint64_t entityID, const Coral::UCChar* inName, glm::vec3* inValue)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<ShaderComponent>() && inValue)
		{
			auto& shader = entity.GetComponent<ShaderComponent>();
			std::string name = ch_u16_to_string(inName);

			auto it = std::find_if(shader.Uniforms.begin(), shader.Uniforms.end(),
								   [&](const auto& u) { return u.Name == name; });

			if (it != shader.Uniforms.end())
			{
				it->Value = *inValue;
			}
			else
			{
				ShaderUniform uniform;
				uniform.Name = name;
				uniform.Value = *inValue;
				shader.Uniforms.push_back(uniform);
			}
		}
	}
	bool Shader_GetEnabled(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<ShaderComponent>() ? entity.GetComponent<ShaderComponent>().Enabled
																: false;
	}
	void Shader_SetEnabled(uint64_t entityID, bool enabled)
	{
		Entity entity = GetEntity(entityID);
		if (entity && entity.HasComponent<ShaderComponent>())
		{
			entity.GetComponent<ShaderComponent>().Enabled = enabled;
		}
	}

	// ── NetworkIdentity ──────────────────────────────────────────────────

	uint64_t NetworkIdentityComponent_GetNetworkID(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<Chained::NetworkIdentityComponent>()
				   ? entity.GetComponent<Chained::NetworkIdentityComponent>().NetworkID
				   : 0;
	}

	bool NetworkIdentityComponent_GetIsOwner(uint64_t entityID)
	{
		Entity entity = GetEntity(entityID);
		return entity && entity.HasComponent<Chained::NetworkIdentityComponent>()
				   ? entity.GetComponent<Chained::NetworkIdentityComponent>().IsOwner
				   : true;
	}

} // namespace Chained
