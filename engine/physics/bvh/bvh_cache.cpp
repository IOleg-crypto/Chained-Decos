#include "bvh_cache.h"

#include "bvh.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/core/log.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/core/thread_pool.h"
#include <glm/glm.hpp>

namespace CHEngine
{
void BVHCache::Init()
{
    if (m_Initialized)
    {
        return;
    }

    m_Initialized = true;
}

void BVHCache::Shutdown()
{
    if (!m_Initialized)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ByPath.clear();
    m_InProgress.clear();
    m_Stats = {};
    m_Initialized = false;
}

bool BVHCache::IsInitialized() const
{
    return m_Initialized;
}

std::shared_ptr<BVH> BVHCache::GetOrBuild(const std::string& path)
{
    if (!m_Initialized || path.empty())
    {
        return nullptr;
    }

    auto handle = ServiceLocator::Get<AssetManager>().ResolveToHandle(path, ModelAsset::GetStaticType());
    auto asset = ServiceLocator::Get<AssetManager>().Get<ModelAsset>(handle);
    if (!asset || asset->GetState() != AssetState::Ready)
    {
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_ByPath.find(path);
        if (it != m_ByPath.end())
        {
            ++m_Stats.Hits;
            return it->second;
        }

        // Check if we are already building this BVH async
        if (m_InProgress.find(path) != m_InProgress.end())
        {
            ++m_Stats.Misses;
            return nullptr;
        }

        m_InProgress.insert(path);
        ++m_Stats.Misses;
    }

    ThreadPool::Get().QueueTask([this, path, asset]() {
        auto built = BuildFromModelAsset(asset);
        
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (built)
        {
            m_ByPath[path] = built;
            ++m_Stats.Builds;
        }
        m_InProgress.erase(path);
    });

    return nullptr;
}

void BVHCache::Put(const std::string& path, std::shared_ptr<BVH> bvh)
{
    if (!m_Initialized || path.empty() || !bvh)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ByPath[path] = std::move(bvh);
}

void BVHCache::Invalidate(const std::string& path)
{
    if (!m_Initialized || path.empty())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ByPath.erase(path);
}

void BVHCache::Clear()
{
    if (!m_Initialized)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ByPath.clear();
    m_InProgress.clear();
}

BVHCache::Stats BVHCache::GetStats() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Stats;
}

std::shared_ptr<BVH> BVHCache::BuildFromModelAsset(const std::shared_ptr<ModelAsset>& asset)
{
    if (!asset)
    {
        return nullptr;
    }

    CH_CORE_INFO("Physics: Building BVH for '{}'", asset->GetPath());

    const auto& instances = asset->GetInstances();
    const auto& rawMeshes = asset->GetRawMeshes();

    std::vector<CollisionTriangle> allTris;
    for (const auto& inst : instances)
    {
        if (inst.meshIndex < 0 || inst.meshIndex >= (int)rawMeshes.size())
        {
            continue;
        }

        const RawMesh& raw = rawMeshes[inst.meshIndex];
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

            v0 = glm::vec3(inst.localTransform * glm::vec4(v0, 1.0f));
            v1 = glm::vec3(inst.localTransform * glm::vec4(v1, 1.0f));
            v2 = glm::vec3(inst.localTransform * glm::vec4(v2, 1.0f));

            allTris.emplace_back(v0, v1, v2, inst.meshIndex);
        }
    }

    if (allTris.empty())
    {
        return nullptr;
    }

    return BVH::Build(std::move(allTris));
}
} // namespace CHEngine
