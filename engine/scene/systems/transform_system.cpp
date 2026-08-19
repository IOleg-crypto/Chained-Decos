#include "transform_system.h"

namespace Chained::TransformSystem
{
	const glm::vec3& GetTranslation(const TransformComponent& tc)
	{
		return tc.Translation;
	}

	const glm::vec3& GetRotation(const TransformComponent& tc)
	{
		return tc.Rotation;
	}

	const glm::quat& GetRotationQuat(const TransformComponent& tc)
	{
		return tc.RotationQuat;
	}

	const glm::vec3& GetScale(const TransformComponent& tc)
	{
		return tc.Scale;
	}

	glm::mat4 ComputeLocalMatrix(const TransformComponent& tc)
	{
		return glm::translate(glm::mat4(1.0f), tc.Translation) * glm::toMat4(tc.RotationQuat) *
			   glm::scale(glm::mat4(1.0f), tc.Scale);
	}

	glm::mat4 ComputeInterpolatedMatrix(const TransformComponent& tc, float alpha)
	{
		glm::vec3 interpolatedTranslation = glm::mix(tc.PrevTranslation, tc.Translation, alpha);
		glm::quat interpolatedRotation = glm::slerp(tc.PrevRotationQuat, tc.RotationQuat, alpha);
		glm::vec3 interpolatedScale = glm::mix(tc.PrevScale, tc.Scale, alpha);

		return glm::translate(glm::mat4(1.0f), interpolatedTranslation) * glm::toMat4(interpolatedRotation) *
			   glm::scale(glm::mat4(1.0f), interpolatedScale);
	}

	void SetTranslation(TransformComponent& tc, const glm::vec3& translation)
	{
		tc.Translation = translation;
		tc.TransformChanged = true;
	}

	void SetRotation(TransformComponent& tc, const glm::vec3& eulerAngles)
	{
		tc.Rotation = eulerAngles;
		tc.RotationQuat = glm::quat(eulerAngles);
		tc.TransformChanged = true;
	}

	void SetScale(TransformComponent& tc, const glm::vec3& scale)
	{
		tc.Scale = scale;
		tc.TransformChanged = true;
	}

	void SetRotationQuat(TransformComponent& tc, const glm::quat& rotationQuat)
	{
		tc.RotationQuat = rotationQuat;
		tc.Rotation = glm::eulerAngles(rotationQuat);
		tc.TransformChanged = true;
	}

} // namespace Chained::TransformSystem
