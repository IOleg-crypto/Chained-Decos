#ifndef CH_RAYCAST_QUERY_H
#define CH_RAYCAST_QUERY_H

#include "engine/physics/raycast_result.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Chained
{

// Editor and gameplay raycasting helpers (EnTT registry-level, not Jolt).
// Used when you need to pick an entity by collider type without a running Jolt world.
class RaycastQuery
{
public:
    static RaycastResult Raycast(entt::registry& registry, Ray ray);

private:
    // Slab-test ray vs axis-aligned box. Returns the closest hit distance in t.
    static bool RayAABB(glm::vec3 origin, glm::vec3 dir, glm::vec3 min, glm::vec3 max, float& t, glm::vec3& normal);
};

} // namespace Chained

#endif // CH_RAYCAST_QUERY_H
