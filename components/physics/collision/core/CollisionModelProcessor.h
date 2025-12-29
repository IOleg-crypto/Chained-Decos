#ifndef COLLISIONMODELPROCESSOR_H
#define COLLISIONMODELPROCESSOR_H

#include "components/physics/collision/colsystem/CollisionSystem.h"
#include "scene/resources/model/Model.h"
#include "scene/resources/model/ModelConfig.h"
#include <memory>
#include <string>
#include <vector>

struct CollisionConfig
{
    int maxPrecisePerModel = 50;
};

class CollisionModelProcessor
{
public:
    explicit CollisionModelProcessor(CollisionConfig config = CollisionConfig());

    bool AnalyzeModelShape(const Model &model, const std::string &modelName);
    bool AnalyzeGeometryIrregularity(const Model &model);

    std::shared_ptr<Collision> CreateBaseCollision(const Model &model, const std::string &modelName,
                                                   const ModelFileConfig *config,
                                                   bool needsPreciseCollision);

    Collision CreatePreciseInstanceCollision(const Model &model, Vector3 position, float scale,
                                             const ModelFileConfig *config);

    Collision CreatePreciseInstanceCollisionFromCached(const Collision &cachedCollision,
                                                       Vector3 position, float scale);

    Collision CreateSimpleAABBInstanceCollision(const Collision &cachedCollision,
                                                const Vector3 &position, float scale);

    std::string MakeCollisionCacheKey(const std::string &modelName, float scale) const;

private:
    CollisionConfig m_config;
};

#endif // COLLISIONMODELPROCESSOR_H
