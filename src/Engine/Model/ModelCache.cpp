#include "ModelCache.h"
#include <algorithm>
#include <raylib.h>

ModelCache::~ModelCache() { Clear(); }

Model *ModelCache::GetModel(const std::string &name)
{
    auto it = m_cache.find(name);
    if (it != m_cache.end())
    {
        it->second.UpdateAccess();
        m_hitCount++;
        return it->second.model.get();
    }

    m_missCount++;
    return nullptr;
}

bool ModelCache::AddModel(const std::string &name, Model &&model)
{
    // Check if we exceed the limit
    if (m_cache.size() >= m_maxCacheSize)
    {
        EvictLeastRecentlyUsed();
    }

    auto info = std::make_unique<CachedModelInfo>();
    info->model = std::make_unique<Model>(std::move(model));
    info->memorySize = EstimateModelSize(*info->model);
    info->UpdateAccess();

    m_cache[name] = std::move(*info);

    TraceLog(LOG_INFO, "Added model '%s' to cache (size: %zu KB)", name.c_str(),
             info->memorySize / 1024);

    return true;
}

bool ModelCache::RemoveModel(const std::string &name)
{
    auto it = m_cache.find(name);
    if (it != m_cache.end())
    {
        // Free Raylib resources
        if (it->second.model)
        {
            UnloadModel(*it->second.model);
        }
        m_cache.erase(it);
        TraceLog(LOG_INFO, "Removed model '%s' from cache", name.c_str());
        return true;
    }
    return false;
}

void ModelCache::Clear()
{
    for (auto &[name, info] : m_cache)
    {
        if (info.model)
        {
            UnloadModel(*info.model);
        }
    }
    m_cache.clear();
    TraceLog(LOG_INFO, "Model cache cleared");
}

size_t ModelCache::GetTotalMemoryUsage() const
{
    size_t total = 0;
    for (const auto &[name, info] : m_cache)
    {
        total += info.memorySize;
    }
    return total;
}

float ModelCache::GetHitRate() const
{
    int total = m_hitCount + m_missCount;
    return total > 0 ? (float)m_hitCount / total : 0.0f;
}

void ModelCache::CleanupUnusedModels(int maxAgeSeconds)
{
    auto now = std::chrono::steady_clock::now();
    auto maxAge = std::chrono::seconds(maxAgeSeconds);

    auto it = m_cache.begin();
    while (it != m_cache.end())
    {
        auto age = now - it->second.lastAccessed;

        // Don't delete frequently used models
        if (!it->second.isFrequentlyUsed && age > maxAge)
        {
            TraceLog(LOG_INFO, "Evicting unused model '%s' (age: %d seconds)", it->first.c_str(),
                     (int)std::chrono::duration_cast<std::chrono::seconds>(age).count());

            if (it->second.model)
            {
                UnloadModel(*it->second.model);
            }
            it = m_cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ModelCache::PrintCacheStats() const
{
    TraceLog(LOG_INFO, "=== Model Cache Statistics ===");
    TraceLog(LOG_INFO, "Cache size: %zu/%zu models", m_cache.size(), m_maxCacheSize);
    TraceLog(LOG_INFO, "Memory usage: %.2f MB", GetTotalMemoryUsage() / (1024.0f * 1024.0f));
    TraceLog(LOG_INFO, "Hit rate: %.1f%% (%d hits, %d misses)", GetHitRate() * 100, m_hitCount,
             m_missCount);

    // Top 5 most frequently used models
    std::vector<std::pair<std::string, int>> usage;
    for (const auto &[name, info] : m_cache)
    {
        usage.push_back({name, info.accessCount});
    }

    std::sort(usage.begin(), usage.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    TraceLog(LOG_INFO, "Top models by usage:");
    for (int i = 0; i < std::min(5, (int)usage.size()); i++)
    {
        TraceLog(LOG_INFO, "  %d. %s (%d accesses)", i + 1, usage[i].first.c_str(),
                 usage[i].second);
    }
}

// Private methods
size_t ModelCache::EstimateModelSize(const Model &model) const
{
    size_t size = 0;

    // Approximate model size estimation
    for (int i = 0; i < model.meshCount; i++)
    {
        const Mesh &mesh = model.meshes[i];
        size += mesh.vertexCount * 3 * sizeof(float);            // vertices
        size += mesh.vertexCount * 3 * sizeof(float);            // normals
        size += mesh.vertexCount * 2 * sizeof(float);            // texcoords
        size += mesh.triangleCount * 3 * sizeof(unsigned short); // indices
    }

    return size;
}

void ModelCache::EvictLeastRecentlyUsed()
{
    if (m_cache.empty())
        return;

    // Find the least recently used model
    auto oldest = std::min_element(m_cache.begin(), m_cache.end(), [](const auto &a, const auto &b)
                                   { return a.second.lastAccessed < b.second.lastAccessed; });

    if (oldest != m_cache.end())
    {
        TraceLog(LOG_INFO, "Evicting LRU model: %s", oldest->first.c_str());
        RemoveModel(oldest->first);
    }
}