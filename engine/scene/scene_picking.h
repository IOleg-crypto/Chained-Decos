#ifndef CH_SCENE_PICKING_H
#define CH_SCENE_PICKING_H

#include "engine/core/ch_structures.h"
#include "engine/graphics/api/camera_types.h"
#include "engine/scene/entity.h"

namespace CHEngine
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

    // Creates a ray from viewport coordinates
    static Ray CreateRayFromViewport(const CHEngine::Camera3D& camera, const glm::vec2& mousePosition,
                                     const glm::vec2& viewportSize);
};

} // namespace CHEngine

#endif // CH_SCENE_PICKING_H
