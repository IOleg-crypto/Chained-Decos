#ifndef CH_RAYCAST_RESULT_H
#define CH_RAYCAST_RESULT_H

#include "entt/entt.hpp"
#include <glm/glm.hpp>

namespace Chained
{

	struct Ray
	{
		glm::vec3 position = {0.0f, 0.0f, 0.0f};
		glm::vec3 direction = {0.0f, 0.0f, 1.0f};
	};

	struct RaycastResult
	{
		bool Hit = false;
		float Distance = 0.0f;
		glm::vec3 Position = {0.0f, 0.0f, 0.0f};
		entt::entity Entity = entt::null;
	};
} // namespace Chained

#endif // CH_RAYCAST_RESULT_H
