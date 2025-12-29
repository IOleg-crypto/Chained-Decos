#include "CollisionModelProcessor.h"
#include "core/Log.h"
#include <algorithm>
#include <cmath>
#include <raymath.h>

CollisionModelProcessor::CollisionModelProcessor(CollisionConfig config) : m_config(config)
{
}

bool CollisionModelProcessor::AnalyzeModelShape(const Model &model, const std::string &modelName)
{
    try
    {
        BoundingBox bounds = GetModelBoundingBox(model);
        Vector3 size = {bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y,
                        bounds.max.z - bounds.min.z};

        float maxDim = fmaxf(fmaxf(size.x, size.y), size.z);
        float minDim = fminf(fminf(size.x, size.y), size.z);

        if (maxDim <= 0.0f || minDim <= 0.0f)
            return false;

        float aspectRatioXY = size.x / size.y;
        float aspectRatioXZ = size.x / size.z;
        float aspectRatioYZ = size.y / size.z;

        const float MAX_RECTANGULAR_RATIO = 10.0f;
        bool isRectangular = (aspectRatioXY <= MAX_RECTANGULAR_RATIO &&
                              aspectRatioXY >= 1.0f / MAX_RECTANGULAR_RATIO) &&
                             (aspectRatioXZ <= MAX_RECTANGULAR_RATIO &&
                              aspectRatioXZ >= 1.0f / MAX_RECTANGULAR_RATIO) &&
                             (aspectRatioYZ <= MAX_RECTANGULAR_RATIO &&
                              aspectRatioYZ >= 1.0f / MAX_RECTANGULAR_RATIO);

        int totalTriangles = 0;
        for (int i = 0; i < model.meshCount; i++)
        {
            totalTriangles += model.meshes[i].triangleCount;
        }

        if (totalTriangles <= 12 && isRectangular)
            return false;
        if (totalTriangles > 100 || !isRectangular)
            return true;

        if (totalTriangles > 12 && totalTriangles <= 100)
        {
            return AnalyzeGeometryIrregularity(model) || !isRectangular;
        }

        return false;
    }
    catch (...)
    {
        return false;
    }
}

bool CollisionModelProcessor::AnalyzeGeometryIrregularity(const Model &model)
{
    int irregularFeatures = 0;
    int totalFaces = 0;

    for (int i = 0; i < model.meshCount; i++)
    {
        const Mesh &mesh = model.meshes[i];
        if (!mesh.vertices || mesh.vertexCount == 0)
            continue;
        totalFaces += mesh.triangleCount;
        if (mesh.vertexCount > mesh.triangleCount * 2)
            irregularFeatures++;
    }

    return totalFaces > 0 && (irregularFeatures * 3) > totalFaces;
}

std::shared_ptr<Collision>
CollisionModelProcessor::CreateBaseCollision(const Model &model, const std::string &modelName,
                                             const ModelFileConfig *config,
                                             bool needsPreciseCollision)
{
    auto collision = std::make_shared<Collision>();

    if (model.meshCount == 0)
    {
        BoundingBox bb = GetModelBoundingBox(model);
        collision->Update(Vector3Scale(Vector3Add(bb.min, bb.max), 0.5f),
                          Vector3Scale(Vector3Subtract(bb.max, bb.min), 0.5f));
        collision->SetCollisionType(CollisionType::AABB_ONLY);
        return collision;
    }

    collision->BuildFromModel(const_cast<Model *>(&model), MatrixIdentity());

    if (needsPreciseCollision && config)
    {
        CollisionType type = CollisionType::HYBRID_AUTO;
        if (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE)
            type = CollisionType::TRIANGLE_PRECISE;
        else if (config->collisionPrecision == CollisionPrecision::BVH_ONLY)
            type = CollisionType::BVH_ONLY;
        collision->SetCollisionType(type);
    }
    else
    {
        collision->SetCollisionType(CollisionType::AABB_ONLY);
    }

    return collision;
}

Collision CollisionModelProcessor::CreatePreciseInstanceCollision(const Model &model,
                                                                  Vector3 position, float scale,
                                                                  const ModelFileConfig *config)
{
    Collision instance;
    Matrix transform = MatrixMultiply(MatrixScale(scale, scale, scale),
                                      MatrixTranslate(position.x, position.y, position.z));
    instance.BuildFromModel(const_cast<Model *>(&model), transform);
    instance.SetCollisionType(CollisionType::BVH_ONLY);
    return instance;
}

Collision
CollisionModelProcessor::CreatePreciseInstanceCollisionFromCached(const Collision &cachedCollision,
                                                                  Vector3 position, float scale)
{
    Collision instance;
    Matrix transform = MatrixMultiply(MatrixScale(scale, scale, scale),
                                      MatrixTranslate(position.x, position.y, position.z));

    for (const auto &t : cachedCollision.GetTriangles())
    {
        instance.AddTriangle(CollisionTriangle(Vector3Transform(t.V0(), transform),
                                               Vector3Transform(t.V1(), transform),
                                               Vector3Transform(t.V2(), transform)));
    }

    instance.UpdateAABBFromTriangles();
    instance.InitializeBVH();
    instance.SetCollisionType(CollisionType::BVH_ONLY);
    return instance;
}

Collision
CollisionModelProcessor::CreateSimpleAABBInstanceCollision(const Collision &cachedCollision,
                                                           const Vector3 &position, float scale)
{
    Vector3 center = Vector3Add(Vector3Scale(cachedCollision.GetCenter(), scale), position);
    Vector3 halfSize = Vector3Scale(cachedCollision.GetSize(), 0.5f * scale);
    Collision instance(center, halfSize);
    instance.SetCollisionType(CollisionType::AABB_ONLY);
    return instance;
}

std::string CollisionModelProcessor::MakeCollisionCacheKey(const std::string &modelName,
                                                           float scale) const
{
    int scaledInt = static_cast<int>(roundf(scale * 1000.0f));
    return modelName + "_s" + std::to_string(scaledInt);
}
