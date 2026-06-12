#ifndef CH_RAYCAST_QUERY_H
#define CH_RAYCAST_QUERY_H

#include "engine/foundation/color.h"
#include "raycast_result.h"
#include "collision_core.h"
#include <entt/entt.hpp>

namespace Chained
{
// Editor and gameplay raycasting helpers.
class RaycastQuery
{
public:
    static RaycastResult Raycast(entt::registry& registry, Ray ray);

private:
    static bool RayAABB(glm::vec3 origin, glm::vec3 dir, glm::vec3 min, glm::vec3 max, float& t, glm::vec3& normal);
};
} // namespace Chained

#endif // CH_RAYCAST_QUERY_H
