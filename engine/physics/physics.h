#ifndef CH_PHYSICS_H
#define CH_PHYSICS_H

#include <entt/entt.hpp>
#include <raylib.h>

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

    // Collision Detection Helpers
    static bool CheckAABB(const Vector3 &min1, const Vector3 &max1, const Vector3 &min2,
                          const Vector3 &max2);

    static RaycastResult Raycast(Scene *scene, Ray ray);
};
} // namespace CHEngine

#endif // CH_PHYSICS_H
