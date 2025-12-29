#include "CollisionPredictionCache.h"
#include <functional>

CollisionPredictionCache::CollisionPredictionCache(size_t maxEntries, size_t lifetime)
    : m_maxEntries(maxEntries), m_lifetime(lifetime)
{
}

bool CollisionPredictionCache::TryGet(const Collision &target, size_t currentFrame,
                                      Vector3 &outResponse, bool &outHit)
{
    size_t hash = GetHash(target);
    auto it = m_cache.find(hash);
    if (it != m_cache.end() && (currentFrame - it->second.frameCount) < m_lifetime)
    {
        outResponse = it->second.response;
        outHit = it->second.hasCollision;
        return true;
    }
    return false;
}

void CollisionPredictionCache::Set(const Collision &target, size_t currentFrame, bool hit,
                                   const Vector3 &response)
{
    size_t hash = GetHash(target);
    m_cache[hash] = {hit, response, currentFrame};
    ManageSize();
}

void CollisionPredictionCache::Update(size_t currentFrame)
{
    if (currentFrame % 60 == 0)
    {
        auto it = m_cache.begin();
        while (it != m_cache.end())
        {
            if (currentFrame - it->second.frameCount > m_lifetime)
                it = m_cache.erase(it);
            else
                ++it;
        }
    }
}

void CollisionPredictionCache::Clear()
{
    m_cache.clear();
}

size_t CollisionPredictionCache::GetHash(const Collision &target) const
{
    Vector3 min = target.GetMin();
    Vector3 max = target.GetMax();
    size_t h = std::hash<float>{}(min.x) ^ std::hash<float>{}(min.y) ^ std::hash<float>{}(min.z);
    h = h * 31 + std::hash<float>{}(max.x) ^ std::hash<float>{}(max.y) ^ std::hash<float>{}(max.z);
    return h;
}

void CollisionPredictionCache::ManageSize()
{
    if (m_cache.size() > m_maxEntries)
    {
        m_cache.erase(m_cache.begin());
    }
}
