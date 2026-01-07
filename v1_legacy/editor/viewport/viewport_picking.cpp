#include "viewport_picking.h"
#include "engine/scene/core/scene.h"
#include "engine/scene/ecs/components/render_component.h"
#include "engine/scene/ecs/components/transform_component.h"
#include <raymath.h>

namespace CHEngine
{

entt::entity ViewportPicking::PickEntity(ImVec2 mousePos, ImVec2 viewportPos, ImVec2 viewportSize,
                                         const Camera3D &camera, Scene &scene)
{
    Ray ray = GetMouseRay(mousePos, viewportPos, viewportSize, camera);
    entt::entity closestEntity = entt::null;
    float minDistance = FLT_MAX;

    auto &registry = scene.GetRegistry();
    auto view = registry.view<TransformComponent, RenderComponent>();

    for (auto entity : view)
    {
        auto &transform = view.get<TransformComponent>(entity);
        auto &render = view.get<RenderComponent>(entity);

        if (!render.model)
            continue;

        // Simple AABB collision for picking
        // In a real engine, we'd use per-mesh picking or physics raycasts
        BoundingBox box = GetModelBoundingBox(*render.model);

        // Transform BoundingBox
        Matrix mat = MatrixMultiply(
            MatrixMultiply(MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z),
                           MatrixRotateXYZ(transform.rotation)),
            MatrixTranslate(transform.position.x, transform.position.y, transform.position.z));

        // This is a simplification. For accurate ray-aabb we need to transform the ray to local
        // space or transform the box. For now, let's just use the position and a rough size.
        float radius = 1.0f; // Default radius
        RayCollision collision = GetRayCollisionSphere(ray, transform.position, radius);

        if (collision.hit && collision.distance < minDistance)
        {
            minDistance = collision.distance;
            closestEntity = entity;
        }
    }

    return closestEntity;
}

Ray ViewportPicking::GetMouseRay(ImVec2 mousePos, ImVec2 viewportPos, ImVec2 viewportSize,
                                 const Camera3D &camera)
{
    // Hazel-style: proper NDC conversion with Y-flip for OpenGL
    Vector2 localMouse = {mousePos.x - viewportPos.x, mousePos.y - viewportPos.y};
    float ndcX = (2.0f * localMouse.x) / viewportSize.x - 1.0f;
    float ndcY = 1.0f - (2.0f * localMouse.y) / viewportSize.y;

    // Calculate ray direction in view space
    float aspectRatio = viewportSize.x / viewportSize.y;
    float tanHalfFovy = tanf(camera.fovy * 0.5f * DEG2RAD);

    Vector3 rayDir = {ndcX * aspectRatio * tanHalfFovy, ndcY * tanHalfFovy, 1.0f};

    // Transform to world space
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    Vector3 worldRayDir =
        Vector3Normalize({rayDir.x * right.x + rayDir.y * up.x + rayDir.z * forward.x,
                          rayDir.x * right.y + rayDir.y * up.y + rayDir.z * forward.z});

    return {camera.position, worldRayDir};
}

} // namespace CHEngine
