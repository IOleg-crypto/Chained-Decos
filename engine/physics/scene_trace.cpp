#include "scene_trace.h"
#include "physics.h"
#include "bvh/bvh.h"
#include "cfloat"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/graphics/asset_manager.h"
#include "engine/scene/project.h"
#include "raymath.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace CHEngine
{
// Helper to convert Raylib Matrix to GLM mat4
static inline glm::mat4 ToGLM(Matrix m)
{
    glm::mat4 result;
    result[0][0] = m.m0; result[0][1] = m.m1; result[0][2] = m.m2; result[0][3] = m.m3;
    result[1][0] = m.m4; result[1][1] = m.m5; result[1][2] = m.m6; result[1][3] = m.m7;
    result[2][0] = m.m8; result[2][1] = m.m9; result[2][2] = m.m10; result[2][3] = m.m11;
    result[3][0] = m.m12; result[3][1] = m.m13; result[3][2] = m.m14; result[3][3] = m.m15;
    return result;
}

// Robust Ray-AABB intersection (Slab method)
static bool RayAABB(glm::vec3 origin, glm::vec3 dir, glm::vec3 min, glm::vec3 max, float& t, glm::vec3& normal)
{
    glm::vec3 invDir = 1.0f / dir;
    glm::vec3 t0 = (min - origin) * invDir;
    glm::vec3 t1 = (max - origin) * invDir;
    
    glm::vec3 tMin = glm::min(t0, t1);
    glm::vec3 tMax = glm::max(t0, t1);
    
    float fmin = glm::max(glm::max(tMin.x, tMin.y), tMin.z);
    float fmax = glm::min(glm::min(tMax.x, tMax.y), tMax.z);
    
    if (fmax >= glm::max(0.0f, fmin))
    {
        t = fmin;
        // Calculate normal in local space
        if (fmin == tMin.x) normal = glm::vec3(dir.x > 0 ? -1 : 1, 0, 0);
        else if (fmin == tMin.y) normal = glm::vec3(0, dir.y > 0 ? -1 : 1, 0);
        else normal = glm::vec3(0, 0, dir.z > 0 ? -1 : 1);
        return true;
    }
    return false;
}

RaycastResult SceneTrace::Raycast(Scene *scene, Ray ray, Physics* physics)
{
    RaycastResult result;
    result.Hit = false;
    result.Distance = FLT_MAX;
    result.Entity = entt::null;

    glm::vec3 rayOrigin = {ray.position.x, ray.position.y, ray.position.z};
    glm::vec3 rayDir = glm::normalize(glm::vec3(ray.direction.x, ray.direction.y, ray.direction.z));

    auto view = scene->GetRegistry().view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto &entityTransform = view.get<TransformComponent>(entity);
        auto &colliderComp = view.get<ColliderComponent>(entity);

        if (!colliderComp.Enabled) continue;

        // Transform ray to local space
        glm::mat4 modelMatrix = ToGLM(entityTransform.GetTransform());
        glm::mat4 invMatrix = glm::inverse(modelMatrix);

        glm::vec3 localOrigin = glm::vec3(invMatrix * glm::vec4(rayOrigin, 1.0f));
        glm::vec3 localDir = glm::normalize(glm::vec3(invMatrix * glm::vec4(rayDir, 0.0f)));

        if (colliderComp.Type == ColliderType::Box)
        {
            float t = 0;
            glm::vec3 localNormal;
            glm::vec3 boxMin = {colliderComp.Offset.x, colliderComp.Offset.y, colliderComp.Offset.z};
            glm::vec3 boxMax = boxMin + glm::vec3(colliderComp.Size.x, colliderComp.Size.y, colliderComp.Size.z);

            if (RayAABB(localOrigin, localDir, boxMin, boxMax, t, localNormal))
            {
                glm::vec3 hitPosWorld = glm::vec3(modelMatrix * glm::vec4(localOrigin + localDir * t, 1.0f));
                float distWorld = glm::distance(rayOrigin, hitPosWorld);

                if (distWorld < result.Distance)
                {
                    result.Hit = true;
                    result.Distance = distWorld;
                    result.Position = {hitPosWorld.x, hitPosWorld.y, hitPosWorld.z};
                    
                    glm::vec3 normalWorld = glm::normalize(glm::vec3(modelMatrix * glm::vec4(localNormal, 0.0f)));
                    result.Normal = {normalWorld.x, normalWorld.y, normalWorld.z};
                    result.Entity = entity;
                }
            }
        }
        else if (colliderComp.Type == ColliderType::Mesh)
        {
            auto modelComp = scene->GetRegistry().try_get<ModelComponent>(entity);
            if (!modelComp || modelComp->ModelPath.empty()) continue;

            auto project = Project::GetActive();
            if (!project || !project->GetAssetManager()) continue;

            auto asset = project->GetAssetManager()->Get<ModelAsset>(modelComp->ModelPath);
            auto bvh = physics->GetBVH(asset.get());
            if (!bvh) continue;

            Ray localRayRaylib = { {localOrigin.x, localOrigin.y, localOrigin.z}, {localDir.x, localDir.y, localDir.z} };
            float t_local = FLT_MAX;
            Vector3 localNormal = {0, 0, 0};
            int localMeshIndex = -1;

            if (bvh->Raycast(localRayRaylib, t_local, localNormal, localMeshIndex))
            {
                glm::vec3 hitPosLocal = localOrigin + localDir * t_local;
                glm::vec3 hitPosWorld = glm::vec3(modelMatrix * glm::vec4(hitPosLocal, 1.0f));
                float distWorld = glm::distance(rayOrigin, hitPosWorld);

                if (distWorld < result.Distance)
                {
                    result.Hit = true;
                    result.Distance = distWorld;
                    result.Position = {hitPosWorld.x, hitPosWorld.y, hitPosWorld.z};

                    glm::vec3 localNormGlm = {localNormal.x, localNormal.y, localNormal.z};
                    glm::vec3 normalWorld = glm::normalize(glm::vec3(modelMatrix * glm::vec4(localNormGlm, 0.0f)));
                    result.Normal = {normalWorld.x, normalWorld.y, normalWorld.z};
                    result.Entity = entity;
                    result.MeshIndex = localMeshIndex;
                }
            }
        }
    }

    return result;
}
} // namespace CHEngine
