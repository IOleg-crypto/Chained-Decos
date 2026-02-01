#include "scene_trace.h"
#include "bvh/bvh.h"
#include "cfloat"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "raymath.h"

namespace CHEngine
{
RaycastResult SceneTrace::Raycast(Scene *scene, Ray ray)
{
    RaycastResult result;
    result.Hit = false;
    result.Distance = FLT_MAX;
    result.Entity = entt::null;

    auto view = scene->GetRegistry().view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto &entityTransform = view.get<TransformComponent>(entity);
        auto &colliderComp = view.get<ColliderComponent>(entity);

        if (!colliderComp.Enabled)
            continue;

        if (colliderComp.Type == ColliderType::Box)
        {
            BoundingBox box;
            Vector3 scaledSize = Vector3Multiply(colliderComp.Size, entityTransform.Scale);
            Vector3 scaledOffset = Vector3Multiply(colliderComp.Offset, entityTransform.Scale);

            box.min = Vector3Add(entityTransform.Translation, scaledOffset);
            box.max = Vector3Add(box.min, scaledSize);

            RayCollision collision = GetRayCollisionBox(ray, box);
            if (collision.hit && collision.distance < result.Distance)
            {
                result.Hit = true;
                result.Distance = collision.distance;
                result.Position = collision.point;
                result.Normal = collision.normal;
                result.Entity = entity;
            }
        }
        else if (colliderComp.Type == ColliderType::Mesh && colliderComp.BVHRoot)
        {
            Matrix modelTransform = entityTransform.GetTransform();
            Matrix invTransform = MatrixInvert(modelTransform);

            Vector3 localOrigin = Vector3Transform(ray.position, invTransform);
            Vector3 localTarget =
                Vector3Transform(Vector3Add(ray.position, ray.direction), invTransform);
            Vector3 localDir = Vector3Normalize(Vector3Subtract(localTarget, localOrigin));

            Ray localRay = {localOrigin, localDir};
            float t_local = FLT_MAX;
            Vector3 localNormal = {0, 0, 0};
            int localMeshIndex = -1;
            if (colliderComp.BVHRoot->Raycast(localRay, t_local, localNormal, localMeshIndex))
            {
                Vector3 hitPosLocal = Vector3Add(localOrigin, Vector3Scale(localDir, t_local));
                Vector3 hitPosWorld = Vector3Transform(hitPosLocal, modelTransform);
                float distWorld = Vector3Distance(ray.position, hitPosWorld);

                if (distWorld < result.Distance)
                {
                    result.Hit = true;
                    result.Distance = distWorld;
                    result.Position = hitPosWorld;

                    Vector3 normalWorld = Vector3Normalize(
                        Vector3Subtract(Vector3Transform(localNormal, modelTransform),
                                        Vector3Transform({0, 0, 0}, modelTransform)));

                    result.Normal = normalWorld;
                    result.Entity = entity;
                    result.MeshIndex = localMeshIndex;
                }
            }
        }
    }

    return result;
}
} // namespace CHEngine
