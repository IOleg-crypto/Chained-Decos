#include "scene_trace.h"
#include "bvh/bvh.h"
#include "cfloat"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "physics.h"
#include "raymath.h"

namespace CHEngine
{

// Robust Ray-AABB intersection (Slab method)
bool SceneTrace::RayAABB(Vector3 origin, Vector3 dir, Vector3 min, Vector3 max, float& t, Vector3& normal)
{
    Vector3 invDir = {1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z};

    Vector3 t0 = Vector3Multiply(Vector3Subtract(min, origin), invDir);
    Vector3 t1 = Vector3Multiply(Vector3Subtract(max, origin), invDir);

    Vector3 tMin = Vector3Min(t0, t1);
    Vector3 tMax = Vector3Max(t0, t1);

    float minVal = fmaxf(fmaxf(tMin.x, tMin.y), tMin.z);
    float maxVal = fminf(fminf(tMax.x, tMax.y), tMax.z);

    if (maxVal >= fmaxf(0.0f, minVal))
    {
        t = minVal;
        // Calculate normal in local space
        if (minVal == tMin.x)
        {
            normal = {(float)(dir.x > 0 ? -1 : 1), 0, 0};
        }
        else if (minVal == tMin.y)
        {
            normal = {0, (float)(dir.y > 0 ? -1 : 1), 0};
        }
        else
        {
            normal = {0, 0, (float)(dir.z > 0 ? -1 : 1)};
        }
        return true;
    }
    return false;
}

RaycastResult SceneTrace::Raycast(Scene* scene, Ray ray)
{
    RaycastResult result;
    result.Hit = false;
    result.Distance = FLT_MAX;
    result.Entity = entt::null;

    Vector3 rayOrigin = ray.position;
    Vector3 rayDir = Vector3Normalize(ray.direction);

    auto view = scene->GetRegistry().view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto& entityTransform = view.get<TransformComponent>(entity);
        auto& colliderComp = view.get<ColliderComponent>(entity);

        if (!colliderComp.Enabled)
        {
            continue;
        }

        // Transform ray to local space
        Matrix modelMatrix = entityTransform.GetTransform();
        Matrix invMatrix = MatrixInvert(modelMatrix);

        Vector3 localOrigin = Vector3Transform(rayOrigin, invMatrix);
        // Better way to transform normal/dir: Remove translation from matrix or w=0
        Vector3 localDir = Vector3Normalize(
            Vector3Subtract(Vector3Transform(rayDir, invMatrix), Vector3Transform(Vector3Zero(), invMatrix)));

        if (colliderComp.Type == ColliderType::Box)
        {
            float t = 0;
            Vector3 localNormal = {0, 0, 0};
            Vector3 boxMin = colliderComp.Offset;
            Vector3 boxMax = Vector3Add(boxMin, colliderComp.Size);

            if (SceneTrace::RayAABB(localOrigin, localDir, boxMin, boxMax, t, localNormal))
            {
                Vector3 hitPosLocal = Vector3Add(localOrigin, Vector3Scale(localDir, t));
                Vector3 hitPosWorld = Vector3Transform(hitPosLocal, modelMatrix);
                float distWorld = Vector3Distance(rayOrigin, hitPosWorld);

                if (distWorld < result.Distance)
                {
                    result.Hit = true;
                    result.Distance = distWorld;
                    result.Position = hitPosWorld;

                    Vector3 normalWorld = Vector3Normalize(Vector3Transform(localNormal, modelMatrix) -
                                                           Vector3Transform(Vector3Zero(), modelMatrix));
                    result.Normal = normalWorld;
                    result.Entity = entity;
                }
            }
        }
        else if (colliderComp.Type == ColliderType::Mesh)
        {
            auto modelComp = scene->GetRegistry().try_get<ModelComponent>(entity);
            if (!modelComp || modelComp->ModelPath.empty())
            {
                continue;
            }

            auto project = Project::GetActive();
            if (!project || !project->GetAssetManager())
            {
                continue;
            }

            auto asset = project->GetAssetManager()->Get<ModelAsset>(modelComp->ModelPath);
            auto bvh = m_Physics->GetBVH(asset.get());
            if (!bvh)
            {
                continue;
            }

            Ray localRayRaylib = {localOrigin, localDir};
            float t_local = FLT_MAX;
            Vector3 localNormal = {0, 0, 0};
            int localMeshIndex = -1;

            if (bvh->Raycast(localRayRaylib, t_local, localNormal, localMeshIndex))
            {
                Vector3 hitPosLocal = Vector3Add(localOrigin, Vector3Scale(localDir, t_local));
                Vector3 hitPosWorld = Vector3Transform(hitPosLocal, modelMatrix);
                float distWorld = Vector3Distance(rayOrigin, hitPosWorld);

                if (distWorld < result.Distance)
                {
                    result.Hit = true;
                    result.Distance = distWorld;
                    result.Position = hitPosWorld;

                    Vector3 normalWorld = Vector3Normalize(Vector3Transform(localNormal, modelMatrix) -
                                                           Vector3Transform(Vector3Zero(), modelMatrix));
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
