#include "scene_picking.h"
#include "engine/assets/asset_manager.h"
#include "engine/scene/components/component_utils.h"
#include "engine/runtime/application.h"
#include "engine/assets/types/model_asset.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include <float.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Chained
{

SceneRaycastResult ScenePicker::Raycast(Scene* scene, const Ray& ray)
{
    SceneRaycastResult finalResult;
    finalResult.Hit = false;
    finalResult.Distance = FLT_MAX;

    if (!scene)
    {
        return finalResult;
    }

    // 1. Physics Picking
    RaycastResult physicsResult = Physics::Raycast(scene, ray);
    if (physicsResult.Hit)
    {
        finalResult.Hit = true;
        finalResult.Distance = physicsResult.Distance;
        finalResult.HitEntity = Entity(physicsResult.Entity, &scene->GetRegistry());
        finalResult.Position = physicsResult.Position;
        finalResult.Normal = physicsResult.Normal;
    }

    // 2. Visual Picking Fallback
    auto modelView = scene->GetRegistry().view<TransformComponent, ModelComponent>();
    modelView.each([&](entt::entity entityID, TransformComponent& tc, ModelComponent& modelComp)
    {
        if (finalResult.Hit && (entt::entity)finalResult.HitEntity == entityID)
        {
            return;
        }

        if (modelComp.ModelPath.empty())
        {
            return;
        }

        AssetManager* assetManager = &AssetManager::Get();

        auto handle = assetManager ? assetManager->ImportAsset(modelComp.ModelPath) : AssetHandle(0);
        auto modelAsset = assetManager ? assetManager->GetAsset<ModelAsset>(handle) : nullptr;
        if (!modelAsset || !modelAsset->IsReady())
        {
            return;
        }

        glm::mat4 modelTransform = ComponentUtils::GetTransform(tc);
        glm::mat4 invTransform = glm::inverse(modelTransform);

        // Transform ray to local space
        Ray localRay;
        localRay.position = glm::vec3(invTransform * glm::vec4(ray.position, 1.0f));
        glm::vec3 localTarget = glm::vec3(invTransform * glm::vec4(ray.position + ray.direction, 1.0f));
        localRay.direction = glm::normalize(localTarget - localRay.position);

        float t_local = FLT_MAX;
        glm::vec3 localNormal = {0, 0, 0};
        int localMeshIndex = -1;

        bool hit = false;
        auto bvh = Physics::GetBVH(modelComp.ModelPath);

        if (bvh)
        {
            hit = bvh->Raycast(localRay, t_local, localNormal, localMeshIndex);
        }
        else
        {
            const auto& rawMeshes = modelAsset->GetRawMeshes();
            for (const auto& raw : rawMeshes)
            {
                if (raw.indices.size() < 3)
                {
                    continue;
                }

                for (size_t i = 0; i + 2 < raw.indices.size(); i += 3)
                {
                    uint32_t i0 = raw.indices[i];
                    uint32_t i1 = raw.indices[i + 1];
                    uint32_t i2 = raw.indices[i + 2];

                    size_t v0Idx = (size_t)i0 * 3;
                    size_t v1Idx = (size_t)i1 * 3;
                    size_t v2Idx = (size_t)i2 * 3;
                    if (v0Idx + 2 >= raw.vertices.size() || v1Idx + 2 >= raw.vertices.size() || v2Idx + 2 >= raw.vertices.size())
                    {
                        continue;
                    }

                    glm::vec3 v0 = {raw.vertices[v0Idx], raw.vertices[v0Idx + 1], raw.vertices[v0Idx + 2]};
                    glm::vec3 v1 = {raw.vertices[v1Idx], raw.vertices[v1Idx + 1], raw.vertices[v1Idx + 2]};
                    glm::vec3 v2 = {raw.vertices[v2Idx], raw.vertices[v2Idx + 1], raw.vertices[v2Idx + 2]};

                    CollisionTriangle tri(v0, v1, v2, 0);
                    float triT = FLT_MAX;
                    glm::vec3 triNormal;
                    if (tri.IntersectsRay(localRay, triT, triNormal) && triT < t_local)
                    {
                        t_local = triT;
                        hit = true;
                        localNormal = triNormal;
                    }
                }
            }
        }

        if (hit)
        {
            glm::vec3 hitPosLocal = localRay.position + localRay.direction * t_local;
            glm::vec3 hitPosWorld = glm::vec3(modelTransform * glm::vec4(hitPosLocal, 1.0f));
            float distWorld = glm::distance(ray.position, hitPosWorld);

            if (distWorld < finalResult.Distance)
            {
                finalResult.Distance = distWorld;
                finalResult.Hit = true;
                finalResult.HitEntity = Entity(entityID, &scene->GetRegistry());
                finalResult.Position = hitPosWorld;
                finalResult.Normal =
                    glm::normalize(glm::vec3(glm::transpose(invTransform) * glm::vec4(localNormal, 0.0f)));
            }
        }
        });

    return finalResult;
}

Ray ScenePicker::CreateRayFromViewport(const Chained::Camera3D& camera, const glm::vec2& mousePosition,
                                       const glm::vec2& viewportSize)
{
    float ndc_x = (2.0f * mousePosition.x) / viewportSize.x - 1.0f;
    float ndc_y = 1.0f - (2.0f * mousePosition.y) / viewportSize.y;

    glm::mat4 projection;
    if (camera.Projection == 0) // Perspective
    {
        projection = glm::perspective(glm::radians(camera.FovY), viewportSize.x / viewportSize.y, 0.01f, 1000.0f);
    }
    else
    {
        float aspect = viewportSize.x / viewportSize.y;
        float h = camera.FovY * 0.5f;
        float w = h * aspect;
        projection = glm::ortho(-w, w, -h, h, 0.01f, 1000.0f);
    }

    glm::mat4 view = glm::lookAt(camera.Position, camera.Target, camera.Up);
    glm::mat4 invVP = glm::inverse(projection * view);

    auto Unproject = [&](float x, float y, float z) -> glm::vec3 {
        glm::vec4 ndc(x, y, z, 1.0f);
        glm::vec4 world = invVP * ndc;
        if (std::abs(world.w) > 1e-6f)
        {
            return glm::vec3(world) / world.w;
        }
        return glm::vec3(0.0f);
    };

    glm::vec3 nearPoint = Unproject(ndc_x, ndc_y, -1.0f);
    glm::vec3 farPoint = Unproject(ndc_x, ndc_y, 1.0f);

    Ray ray;
    ray.position = nearPoint;
    ray.direction = glm::normalize(farPoint - nearPoint);

    return ray;
}

} // namespace Chained
