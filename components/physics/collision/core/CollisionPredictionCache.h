#ifndef COLLISIONPREDICTIONCACHE_H
#define COLLISIONPREDICTIONCACHE_H

#include "components/physics/collision/colsystem/CollisionSystem.h"
#include <unordered_map>

class CollisionPredictionCache
{
public:
    struct Entry
    {
        bool hasCollision;
        Vector3 response;
        size_t frameCount;
    };

    explicit CollisionPredictionCache(size_t maxEntries = 1000, size_t lifetime = 5);

    bool TryGet(const Collision &target, size_t currentFrame, Vector3 &outResponse, bool &outHit);
    void Set(const Collision &target, size_t currentFrame, bool hit, const Vector3 &response);

    void Update(size_t currentFrame);
    void Clear();

private:
    size_t GetHash(const Collision &target) const;
    void ManageSize();

    size_t m_maxEntries;
    size_t m_lifetime;
    std::unordered_map<size_t, Entry> m_cache;
};

#endif // COLLISIONPREDICTIONCACHE_H
