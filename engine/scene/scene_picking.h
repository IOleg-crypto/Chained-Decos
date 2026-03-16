#ifndef CH_SCENE_PICKING_H
#define CH_SCENE_PICKING_H

#include "engine/scene/entity.h"
#include "raylib.h"

namespace CHEngine
{
class Scene;

struct SceneRaycastResult
{
    Entity HitEntity;
    float Distance = 0.0f;
    Vector3 Position = {0.0f, 0.0f, 0.0f};
    Vector3 Normal = {0.0f, 0.0f, 0.0f};
    bool Hit = false;
};

class ScenePicker
{
public:
    static SceneRaycastResult Raycast(Scene* scene, const Ray& ray);

    // Creates a ray from viewport coordinates
    static Ray CreateRayFromViewport(const Camera3D& camera, const Vector2& mousePosition, const Vector2& viewportSize);
};

} // namespace CHEngine

#endif // CH_SCENE_PICKING_H
