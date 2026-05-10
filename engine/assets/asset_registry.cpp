#include "engine/assets/asset_registry.h"

namespace CHEngine
{
    void AssetRegistry::Register(const std::string& resolvedPath, std::shared_ptr<Asset> asset)
    {
        if (!asset) return;

        std::unique_lock lock(m_Mutex);
        AssetHandle handle = asset->GetID();
        m_AssetCache[handle] = asset;
        
        if (!resolvedPath.empty())
        {
            m_PathToHandle[resolvedPath] = handle;
        }
    }

    void AssetRegistry::Unregister(AssetHandle handle)
    {
        std::unique_lock lock(m_Mutex);
        auto it = m_AssetCache.find(handle);
        if (it != m_AssetCache.end())
        {
            // Find and remove path mapping
            for (auto pathIt = m_PathToHandle.begin(); pathIt != m_PathToHandle.end(); )
            {
                if (pathIt->second == handle)
                    pathIt = m_PathToHandle.erase(pathIt);
                else
                    ++pathIt;
            }
            m_AssetCache.erase(it);
        }
    }

    void AssetRegistry::Clear()
    {
        std::unique_lock lock(m_Mutex);
        m_AssetCache.clear();
        m_PathToHandle.clear();
    }

    std::shared_ptr<Asset> AssetRegistry::Get(AssetHandle handle) const
    {
        std::shared_lock lock(m_Mutex);
        auto it = m_AssetCache.find(handle);
        return (it != m_AssetCache.end()) ? it->second : nullptr;
    }

    std::shared_ptr<Asset> AssetRegistry::Get(const std::string& resolvedPath) const
    {
        std::shared_lock lock(m_Mutex);
        auto it = m_PathToHandle.find(resolvedPath);
        if (it != m_PathToHandle.end())
        {
            auto assetIt = m_AssetCache.find(it->second);
            return (assetIt != m_AssetCache.end()) ? assetIt->second : nullptr;
        }
        return nullptr;
    }

    bool AssetRegistry::Contains(AssetHandle handle) const
    {
        std::shared_lock lock(m_Mutex);
        return m_AssetCache.find(handle) != m_AssetCache.end();
    }

    bool AssetRegistry::Contains(const std::string& resolvedPath) const
    {
        std::shared_lock lock(m_Mutex);
        return m_PathToHandle.find(resolvedPath) != m_PathToHandle.end();
    }

    AssetHandle AssetRegistry::GetHandle(const std::string& resolvedPath) const
    {
        std::shared_lock lock(m_Mutex);
        auto it = m_PathToHandle.find(resolvedPath);
        return (it != m_PathToHandle.end()) ? it->second : AssetHandle(0);
    }
}
