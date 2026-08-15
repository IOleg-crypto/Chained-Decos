#ifndef CH_TRANSFORM_SYSTEM_H
#define CH_TRANSFORM_SYSTEM_H

#include "engine/scene/components/core/transform_component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Chained
{
	namespace TransformSystem
	{
		glm::mat4 ComputeLocalMatrix(const TransformComponent& tc);
		glm::mat4 ComputeInterpolatedMatrix(const TransformComponent& tc, float alpha);

		const glm::vec3& GetTranslation(const TransformComponent& tc);
		const glm::vec3& GetRotation(const TransformComponent& tc);
		const glm::quat& GetRotationQuat(const TransformComponent& tc);
		const glm::vec3& GetScale(const TransformComponent& tc);

		void SetTranslation(TransformComponent& tc, const glm::vec3& translation);
		void SetRotation(TransformComponent& tc, const glm::vec3& eulerAngles);
		void SetScale(TransformComponent& tc, const glm::vec3& scale);
		void SetRotationQuat(TransformComponent& tc, const glm::quat& rotationQuat);
	} // namespace TransformSystem
} // namespace Chained

#endif // CH_TRANSFORM_SYSTEM_H
