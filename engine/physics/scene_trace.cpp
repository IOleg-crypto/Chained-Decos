#include "scene_trace.h"
#include "bvh/bvh.h"
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "physics.h"

namespace CHEngine
{

bool SceneTrace::RayAABB(glm::vec3 origin, glm::vec3 dir, glm::vec3 min, glm::vec3 max, float& t, glm::vec3& normal)
{
    glm::vec3 invDir = 1.0f / dir;

    glm::vec3 t0 = (min - origin) * invDir;
    glm::vec3 t1 = (max - origin) * invDir;

    glm::vec3 tMin = glm::min(t0, t1);
    glm::vec3 tMax = glm::max(t0, t1);

    float minVal = std::max(std::max(tMin.x, tMin.y), tMin.z);
    float maxVal = std::min(std::min(tMax.x, tMax.y), tMax.z);

    if (maxVal >= std::max(0.0f, minVal))
    {
        t = minVal;
        if (minVal == tMin.x)
        {
            normal = { (float)(dir.x > 0 ? -1 : 1), 0, 0 };
        }
        else if (minVal == tMin.y)
        {
            normal = { 0, (float)(dir.y > 0 ? -1 : 1), 0 };
        }
        else
        {
            normal = { 0, 0, (float)(dir.z > 0 ? -1 : 1) };
        }
        return true;
    }
    return false;
}

RaycastResult SceneTrace::Raycast(::entt::registry& registry, Ray ray)
{
    RaycastResult result;
    result.Hit = false;
    result.Distance = FLT_MAX;
    result.Entity = entt::null;

    glm::vec3 rayOrigin = ray.position;
    glm::vec3 rayDir = glm::normalize(ray.direction);

    auto view = registry.view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto& entityTransform = view.get<TransformComponent>(entity);
        auto& colliderComp = view.get<ColliderComponent>(entity);

        if (!colliderComp.Enabled)
        {
            continue;
        }

        glm::mat4 modelMatrix = entityTransform.GetTransform();
        glm::mat4 invMatrix = glm::inverse(modelMatrix);

        glm::vec3 localOrigin = glm::vec3(invMatrix * glm::vec4(rayOrigin, 1.0f));
        glm::vec3 localDir = glm::normalize(glm::vec3(invMatrix * glm::vec4(rayDir, 0.0f)));

        if (colliderComp.Type == ColliderType::Box)
        {
            float t = 0;
            glm::vec3 localNormal = { 0, 0, 0 };
            glm::vec3 boxMin = colliderComp.Offset;
            glm::vec3 boxMax = colliderComp.Offset + colliderComp.Size;

            if (SceneTrace::RayAABB(localOrigin, localDir, boxMin, boxMax, t, localNormal))
            {
                glm::vec3 hitPosLocal = localOrigin + localDir * t;
                glm::vec3 hitPosWorld = glm::vec3(modelMatrix * glm::vec4(hitPosLocal, 1.0f));
                float distWorld = glm::distance(rayOrigin, hitPosWorld);

                if (distWorld < result.Distance)
                {
                    result.Hit = true;
                    result.Distance = distWorld;
                    result.Position = hitPosWorld;

                    glm::vec3 normalWorld = glm::normalize(glm::vec3(modelMatrix * glm::vec4(localNormal, 0.0f)));
                    result.Normal = normalWorld;
                    result.Entity = entity;
                }
            }
        }
        else if (colliderComp.Type == ColliderType::Mesh)
        {
            auto modelComp = registry.try_get<ModelComponent>(entity);
            if (!modelComp || modelComp->ModelPath.empty())
            {
                continue;
            }

            auto project = Project::GetActive();
            if (!project)
            {
                continue;
            }

            auto bvh = PhysicsSystem::Get().GetBVH(modelComp->ModelPath);
            if (!bvh)
            {
                continue;
            }

            Ray localRay = { localOrigin, localDir };
            float t_local = FLT_MAX;
            glm::vec3 localNormal = { 0, 0, 0 };
            int localMeshIndex = -1;

            if (bvh->Raycast(localRay, t_local, localNormal, localMeshIndex))
            {
                glm::vec3 hitPosLocal = localOrigin + localDir * t_local;
                glm::vec3 hitPosWorld = glm::vec3(modelMatrix * glm::vec4(hitPosLocal, 1.0f));
                float distWorld = glm::distance(rayOrigin, hitPosWorld);

                if (distWorld < result.Distance)
                {
                    result.Hit = true;
                    result.Distance = distWorld;
                    result.Position = hitPosWorld;

                    glm::vec3 normalWorld = glm::normalize(glm::vec3(modelMatrix * glm::vec4(localNormal, 0.0f)));
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
