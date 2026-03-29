#ifndef CH_SCENE_TRACE_H
#define CH_SCENE_TRACE_H

#include "raycast_result.h"
#include "engine/core/ch_math.h"
#include <entt/entt.hpp>

namespace CHEngine
{


class SceneTrace
{
public:
    static RaycastResult Raycast(::entt::registry& registry, Ray ray);

private:
    static bool RayAABB(glm::vec3 origin, glm::vec3 dir, glm::vec3 min, glm::vec3 max, float& t, glm::vec3& normal);
};
} // namespace CHEngine

#endif // CH_SCENE_TRACE_H
