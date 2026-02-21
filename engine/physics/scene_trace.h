#ifndef CH_SCENE_TRACE_H
#define CH_SCENE_TRACE_H

#include "physics.h"
#include "raylib.h"

namespace CHEngine
{
class Scene;

class Physics;

class SceneTrace
{
public:
    SceneTrace(Physics* physics)
        : m_Physics(physics)
    {
    }

    RaycastResult Raycast(Scene* scene, Ray ray);

private:
    static bool RayAABB(Vector3 origin, Vector3 dir, Vector3 min, Vector3 max, float& t, Vector3& normal);

    Physics* m_Physics;
};
} // namespace CHEngine

#endif // CH_SCENE_TRACE_H
