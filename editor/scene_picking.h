#ifndef CH_SCENE_PICKING_H
#define CH_SCENE_PICKING_H

#include "engine/common/color.h"
#include "engine/physics/raycast_result.h"
#include "engine/graphics/camera_types.h"
#include "engine/scene/entity.h"

namespace Chained
{
class Scene;

struct SceneRaycastResult
{
    Entity HitEntity;
    float Distance = 0.0f;
    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
    glm::vec3 Normal = {0.0f, 0.0f, 0.0f};
    bool Hit = false;
};

class ScenePicker
{
public:
    static SceneRaycastResult Raycast(Scene* scene, const Ray& ray);
    static Ray CreateRayFromViewport(const Chained::Camera3D& camera, const glm::vec2& mousePosition,
                                     const glm::vec2& viewportSize);
};

} // namespace Chained

#endif // CH_SCENE_PICKING_H
