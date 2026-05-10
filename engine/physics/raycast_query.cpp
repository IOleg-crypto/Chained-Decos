#include "raycast_query.h"

#include "bvh/bvh.h"
#include "engine/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "physics.h"
#include <algorithm>
#include <cfloat>
#include "engine/scene/components/component_utils.h"
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>

namespace CHEngine
{
bool RaycastQuery::RayAABB(glm::vec3 origin, glm::vec3 dir, glm::vec3 min, glm::vec3 max, float& t, glm::vec3& normal)
{
    // Slab test for a ray against an axis-aligned box.
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

RaycastResult RaycastQuery::Raycast(entt::registry& registry, Ray ray)
{
    RaycastResult result;
    result.Hit = false;
    result.Distance = FLT_MAX;
    result.Entity = entt::null;

    glm::vec3 rayOrigin = ray.position;
    glm::vec3 rayDir = glm::normalize(ray.direction);

    // Test every enabled collider in local space, then compare results in world space.
    auto view = registry.view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto& entityTransform = view.get<TransformComponent>(entity);
        auto& colliderComp = view.get<ColliderComponent>(entity);

        if (!colliderComp.Enabled)
        {
            continue;
        }

        glm::mat4 modelMatrix = ComponentUtils::GetTransform(entityTransform);
        glm::mat4 invMatrix = glm::inverse(modelMatrix);

        glm::vec3 localOrigin = glm::vec3(invMatrix * glm::vec4(rayOrigin, 1.0f));
        glm::vec3 localDir = glm::normalize(glm::vec3(invMatrix * glm::vec4(rayDir, 0.0f)));

        if (colliderComp.Type == ColliderType::Box)
        {
            float t = 0;
            glm::vec3 localNormal = {0, 0, 0};
            glm::vec3 boxMin = colliderComp.Offset;
            glm::vec3 boxMax = colliderComp.Offset + colliderComp.Size;

            if (RayAABB(localOrigin, localDir, boxMin, boxMax, t, localNormal))
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

            if (!Project::GetActive())
            {
                continue;
            }

            auto bvh = Physics::GetBVH(modelComp->ModelPath);
            if (!bvh)
            {
                continue;
            }

            Ray localRay = {localOrigin, localDir};
            float tLocal = FLT_MAX;
            glm::vec3 localNormal = {0, 0, 0};
            int localMeshIndex = -1;

            if (bvh->Raycast(localRay, tLocal, localNormal, localMeshIndex))
            {
                glm::vec3 hitPosLocal = localOrigin + localDir * tLocal;
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
