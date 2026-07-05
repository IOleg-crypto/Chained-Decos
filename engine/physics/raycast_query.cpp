#include "raycast_query.h"

#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/core/service_locator.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/components/component_utils.h"
#include <algorithm>
#include <cfloat>
#include <glm/glm.hpp>

namespace Chained {

bool RaycastQuery::RayAABB(glm::vec3 origin, glm::vec3 dir, glm::vec3 min, glm::vec3 max, float &t, glm::vec3 &normal) {
    glm::vec3 invDir = 1.0f / dir;
    glm::vec3 t0 = (min - origin) * invDir;
    glm::vec3 t1 = (max - origin) * invDir;

    glm::vec3 tMin = glm::min(t0, t1);
    glm::vec3 tMax = glm::max(t0, t1);

    float nearT = std::max({tMin.x, tMin.y, tMin.z});
    float farT = std::min({tMax.x, tMax.y, tMax.z});

    if (farT < std::max(0.0f, nearT)) {
        return false;
    }

    t = nearT;

    
    if (nearT == tMin.x) {
        normal = {dir.x > 0 ? -1.f : 1.f, 0, 0};
    } else if (nearT == tMin.y) {
        normal = {0, dir.y > 0 ? -1.f : 1.f, 0};
    } else {
        normal = {0, 0, dir.z > 0 ? -1.f : 1.f};
    }

    return true;
}

RaycastResult RaycastQuery::Raycast(entt::registry &registry, Ray ray) {
    RaycastResult result;
    result.Hit = false;
    result.Distance = FLT_MAX;
    result.Entity = entt::null;

    const glm::vec3 rayOrigin = ray.position;
    const glm::vec3 rayDir = glm::normalize(ray.direction);

    auto view = registry.view<TransformComponent, ColliderComponent>();
    for (auto entity : view) {
        auto &tc = view.get<TransformComponent>(entity);
        auto &collider = view.get<ColliderComponent>(entity);

        if (!collider.Enabled) {
            continue;
        }

        
        glm::mat4 modelMatrix = ComponentUtils::GetTransform(tc);
        glm::mat4 invMatrix = glm::inverse(modelMatrix);

        glm::vec3 localOrigin = glm::vec3(invMatrix * glm::vec4(rayOrigin, 1.0f));
        glm::vec3 localDir = glm::normalize(glm::vec3(invMatrix * glm::vec4(rayDir, 0.0f)));

        
        if (collider.Type == ColliderType::Box) {
            float t = 0;
            glm::vec3 localNormal = {0, 0, 0};

            
            glm::vec3 halfSize = collider.Size * 0.5f;
            glm::vec3 boxMin = collider.Offset - halfSize;
            glm::vec3 boxMax = collider.Offset + halfSize;

            if (!RayAABB(localOrigin, localDir, boxMin, boxMax, t, localNormal)) {
                continue;
            }

            
            glm::vec3 hitWorld = glm::vec3(modelMatrix * glm::vec4(localOrigin + localDir * t, 1.0f));
            float dist = glm::distance(rayOrigin, hitWorld);

            if (dist < result.Distance) {
                result.Hit = true;
                result.Distance = dist;
                result.Position = hitWorld;
                result.Normal = glm::normalize(glm::vec3(modelMatrix * glm::vec4(localNormal, 0.0f)));
                result.Entity = entity;
            }
        }
        
        else if (collider.Type == ColliderType::Sphere) {
            
            glm::vec3 oc = localOrigin - collider.Offset;
            float b = glm::dot(oc, localDir);
            float c = glm::dot(oc, oc) - (collider.Radius * collider.Radius);
            float discriminant = b * b - c;

            if (discriminant >= 0.0f) {
                float t = -b - std::sqrt(discriminant);
                if (t < 0.0f) 
                {
                    t = -b + std::sqrt(discriminant);
                }

                if (t >= 0.0f) {
                    glm::vec3 hitWorld = glm::vec3(modelMatrix * glm::vec4(localOrigin + localDir * t, 1.0f));
                    float dist = glm::distance(rayOrigin, hitWorld);

                    if (dist < result.Distance) {
                        glm::vec3 localNormal = glm::normalize((localOrigin + localDir * t) - collider.Offset);

                        result.Hit = true;
                        result.Distance = dist;
                        result.Position = hitWorld;
                        result.Normal = glm::normalize(glm::vec3(modelMatrix * glm::vec4(localNormal, 0.0f)));
                        result.Entity = entity;
                    }
                }
            }
        }
    }

    return result;
}

} // namespace Chained