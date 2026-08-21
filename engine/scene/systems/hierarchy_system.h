#ifndef CH_HIERARCHY_SYSTEM_H
#define CH_HIERARCHY_SYSTEM_H

#include "entt/entt.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace Chained
{

	struct UpdateTask
	{
		entt::entity Entity;
		glm::mat4 ParentTransform;
		bool ParentChanged;
	};

	namespace Hierarchy
	{
		/**
		 * @brief Updates the world-space transforms for all entities in the hierarchy.
		 * Uses an iterative DFS approach with dirty-flag propagation.
		 */
		void UpdateWorldTransforms(entt::registry& reg, const std::vector<entt::entity>& roots);
	} // namespace Hierarchy

} // namespace Chained

#endif // CH_HIERARCHY_SYSTEM_H
