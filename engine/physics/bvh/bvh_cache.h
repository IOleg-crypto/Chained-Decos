#ifndef CH_PHYSICS_BVH_CACHE_H
#define CH_PHYSICS_BVH_CACHE_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace CHEngine
{
class BVH;
class ModelAsset;

class BVHCache
{
public:
    struct Stats
    {
        uint64_t Hits = 0;
        uint64_t Misses = 0;
        uint64_t Builds = 0;
    };

    void Init();
    void Shutdown();
    bool IsInitialized() const;

    std::shared_ptr<BVH> GetOrBuild(const std::string& path);
    void Put(const std::string& path, std::shared_ptr<BVH> bvh);
    void Invalidate(const std::string& path);
    void Clear();

    Stats GetStats() const;

private:
    static std::shared_ptr<BVH> BuildFromModelAsset(const std::shared_ptr<ModelAsset>& asset);

private:
    std::unordered_map<std::string, std::shared_ptr<BVH>> m_ByPath;
    mutable std::mutex m_Mutex;
    Stats m_Stats{};
    bool m_Initialized = false;
};
} // namespace CHEngine

#endif // CH_PHYSICS_BVH_CACHE_H
