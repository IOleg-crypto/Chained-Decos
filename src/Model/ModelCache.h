#ifndef MODELCACHE_H
#define MODELCACHE_H

#include <chrono>
#include <memory>
#include <raylib.h>
#include <string>
#include <unordered_map>

// Information about cached model
struct CachedModelInfo
{
    std::unique_ptr<Model> model;
    std::chrono::steady_clock::time_point lastAccessed;
    int accessCount = 0;
    size_t memorySize = 0; // Approximate memory size
    bool isFrequentlyUsed = false;

    void UpdateAccess()
    {
        lastAccessed = std::chrono::steady_clock::now();
        accessCount++;
        isFrequentlyUsed = accessCount > 5; // Frequent use threshold
    }
};

// Model cache with automatic memory management
class ModelCache
{
public:
    ModelCache() = default;
    ~ModelCache();

    // Basic cache operations
    Model *GetModel(const std::string &name);
    bool AddModel(const std::string &name, Model &&model);
    bool RemoveModel(const std::string &name);
    void Clear();

    // Cache statistics
    size_t GetCacheSize() const { return m_cache.size(); }
    size_t GetTotalMemoryUsage() const;
    float GetHitRate() const;

    // Memory management
    void CleanupUnusedModels(int maxAgeSeconds = 300); // 5 minutes
    void SetMaxCacheSize(size_t maxSize) { m_maxCacheSize = maxSize; }

    // Debug info
    void PrintCacheStats() const;

private:
    std::unordered_map<std::string, CachedModelInfo> m_cache;
    size_t m_maxCacheSize = 50; // Maximum models in cache

    // Statistics
    mutable int m_hitCount = 0;
    mutable int m_missCount = 0;

    size_t EstimateModelSize(const Model &model) const;
    void EvictLeastRecentlyUsed();
};

#endif /* MODELCACHE_H */
