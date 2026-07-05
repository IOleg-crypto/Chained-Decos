#ifndef CH_PHYSICS_H
#define CH_PHYSICS_H

#include "engine/core/engine_module.h"
#include "engine/foundation/base.h"
#include "engine/foundation/timestep.h"
#include "engine/physics/raycast_result.h"
#include "engine/scene/components.h"
#include "iphysics_world.h"
#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Chained
{
class Scene;
class IPhysicsWorld;
struct PhysicsTriangle;

// Per-scene physics context stored in the EnTT registry ctx.
struct PhysicsContext
{
    float Accumulator = 0.0f;
    std::function<void(entt::entity, entt::entity)> CollisionCallback;
};

class CH_API Physics : public EngineModule
{
public:
    Physics();
    virtual ~Physics() override;

    // EngineModule lifecycle
    virtual void Initialize() override;
    virtual void Update(Timestep ts) override
    {
    }
    virtual void Shutdown() override;

    // Core API
    IPhysicsWorld* GetWorld();
    void ResetWorld();
    void InitializeBodies(Scene* scene);
    void Update(Scene* scene, Timestep deltaTime, bool runtime = false);
    RaycastResult Raycast(Scene* scene, Ray ray);

    // Scene context
    PhysicsContext& GetContext(Scene* scene);
    void ResetAccumulator(Scene* scene);
    void ClearContext(Scene* scene);
    void SetCollisionCallback(Scene* scene, std::function<void(entt::entity, entt::entity)> callback);

private:
    void UpdateColliders(Scene* scene);
    void BuildMeshTriangles(const std::string& modelPath, const glm::vec3& scale, std::vector<PhysicsTriangle>& outTriangles);
    // When AutoCalculate=true: builds a temp Jolt MeshShape from the model,
    // calls GetLocalBounds() and writes Size/Offset/Radius/Height back into the collider.
    void ApplyAutoCalculate(entt::entity entity, entt::registry& registry, ColliderComponent& collider, const glm::vec3& scale);

private:
    std::unique_ptr<IPhysicsWorld> m_World;
};

} // namespace Chained

#endif // CH_PHYSICS_H