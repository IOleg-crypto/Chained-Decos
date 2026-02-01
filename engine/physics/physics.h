#ifndef CH_PHYSICS_H
#define CH_PHYSICS_H

#include "entt/entt.hpp"
#include "raylib.h"

namespace CHEngine
{
class Scene;

struct RaycastResult
{
    bool Hit = false;
    float Distance = 0.0f;
    Vector3 Position = {0.0f, 0.0f, 0.0f};
    Vector3 Normal = {0.0f, 0.0f, 0.0f};
    entt::entity Entity = entt::null;
    int MeshIndex = -1;
};

class Physics
{
public:
    static void Init();
    static void Shutdown();

    static void Update(Scene *scene, float deltaTime, bool runtime = false);
    static RaycastResult Raycast(Scene *scene, Ray ray);
};
} // namespace CHEngine

#endif // CH_PHYSICS_H
