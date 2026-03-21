#include "scene_picking.h"
#include "engine/scene/scene.h"
#include "engine/physics/physics.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/scene/components.h"
#include "raymath.h"
#include <float.h>

namespace CHEngine
{

SceneRaycastResult ScenePicker::Raycast(Scene* scene, const Ray& ray)
{
    SceneRaycastResult finalResult;
    finalResult.Hit = false;
    finalResult.Distance = FLT_MAX;

    if (!scene)
        return finalResult;

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

    // 2. Visual Picking Fallback (for models without colliders)
    auto modelView = scene->GetRegistry().view<TransformComponent, ModelComponent>();
    for (auto entityID : modelView)
    {
        if (finalResult.Hit && (entt::entity)finalResult.HitEntity == entityID)
        {
            continue;
        }

        Entity entity(entityID, &scene->GetRegistry());
        auto& modelComp = modelView.get<ModelComponent>(entityID);
        
        if (modelComp.ModelPath.empty())
            continue;

        auto modelAsset = AssetManager::Get().Get<ModelAsset>(modelComp.ModelPath);
        if (!modelAsset || !modelAsset->IsReady())
            continue;

        auto& tc = modelView.get<TransformComponent>(entityID);
        Matrix modelTransform = tc.GetTransform();
        Matrix invTransform = MatrixInvert(modelTransform);

        // Transform ray to local space
        Ray localRay;
        localRay.position = Vector3Transform(ray.position, invTransform);
        Vector3 localTarget = Vector3Transform(Vector3Add(ray.position, ray.direction), invTransform);
        localRay.direction = Vector3Normalize(Vector3Subtract(localTarget, localRay.position));

        float t_local = FLT_MAX;
        Vector3 localNormal = {0, 0, 0};
        int localMeshIndex = -1;

        bool hit = false;
        auto bvh = PhysicsSystem::Get().GetBVH(modelComp.ModelPath);

        if (bvh)
        {
            hit = bvh->Raycast(localRay, t_local, localNormal, localMeshIndex);
        }
        else
        {
            Model& model = modelAsset->GetModel();
            for (int m = 0; m < model.meshCount; m++)
            {
                RayCollision collision = GetRayCollisionMesh(localRay, model.meshes[m], MatrixIdentity());
                if (collision.hit && collision.distance < t_local)
                {
                    t_local = collision.distance;
                    hit = true;
                    localNormal = collision.normal;
                }
            }
        }

        if (hit)
        {
            Vector3 hitPosLocal = Vector3Add(localRay.position, Vector3Scale(localRay.direction, t_local));
            Vector3 hitPosWorld = Vector3Transform(hitPosLocal, modelTransform);
            float distWorld = Vector3Distance(ray.position, hitPosWorld);

            if (distWorld < finalResult.Distance)
            {
                finalResult.Distance = distWorld;
                finalResult.Hit = true;
                finalResult.HitEntity = entity;
                finalResult.Position = hitPosWorld;
                
                // Final result normal transformation
                finalResult.Normal = Vector3Normalize(Vector3Transform(localNormal, MatrixTranspose(invTransform)));
            }
        }
    }

    return finalResult;
}

Ray ScenePicker::CreateRayFromViewport(const Camera3D& camera, const Vector2& mousePosition, const Vector2& viewportSize)
{
    // NDC: [-1,1], Y-up
    float ndc_x = (2.0f * mousePosition.x) / viewportSize.x - 1.0f;
    float ndc_y = 1.0f - (2.0f * mousePosition.y) / viewportSize.y;

    // View-Projection Matrix
    Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, viewportSize.x / viewportSize.y, 0.01f, 1000.0f);
    if (camera.projection == CAMERA_ORTHOGRAPHIC)
    {
        float aspect = viewportSize.x / viewportSize.y;
        float top = camera.fovy / 2.0f;
        float right = top * aspect;
        projection = MatrixOrtho(-right, right, -top, top, 0.01f, 1000.0f);
    }

    Matrix view = GetCameraMatrix(camera);
    Matrix viewProj = MatrixMultiply(view, projection);
    Matrix invViewProj = MatrixInvert(viewProj);

    // Unproject Near/Far points
    auto Unproject = [&](float x, float y, float z) -> Vector3 {
        float coords[4] = {x, y, z, 1.0f};

        float resPoints[4] = {0};
        // Manual 4x4 multiplication for W component
        resPoints[0] = coords[0] * invViewProj.m0 + coords[1] * invViewProj.m4 + coords[2] * invViewProj.m8 +
                       coords[3] * invViewProj.m12;
        resPoints[1] = coords[0] * invViewProj.m1 + coords[1] * invViewProj.m5 + coords[2] * invViewProj.m9 +
                       coords[3] * invViewProj.m13;
        resPoints[2] = coords[0] * invViewProj.m2 + coords[1] * invViewProj.m6 + coords[2] * invViewProj.m10 +
                       coords[3] * invViewProj.m14;
        resPoints[3] = coords[0] * invViewProj.m3 + coords[1] * invViewProj.m7 + coords[2] * invViewProj.m11 +
                       coords[3] * invViewProj.m15;

        if (fabs(resPoints[3]) > 0.00001f)
        {
            return {resPoints[0] / resPoints[3], resPoints[1] / resPoints[3], resPoints[2] / resPoints[3]};
        }
        return {0, 0, 0};
    };

    Vector3 nearPoint = Unproject(ndc_x, ndc_y, -1.0f);
    Vector3 farPoint = Unproject(ndc_x, ndc_y, 1.0f);

    Ray ray;
    ray.position = nearPoint;

    // Manual vector math to avoid potential signature issues
    float dx = farPoint.x - nearPoint.x;
    float dy = farPoint.y - nearPoint.y;
    float dz = farPoint.z - nearPoint.z;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);

    if (len > 0.0f)
    {
        ray.direction = {dx / len, dy / len, dz / len};
    }
    else
    {
        ray.direction = {0, 0, -1}; // Default forward
    }

    return ray;
}

} // namespace CHEngine
