#ifndef CH_ASSET_REGISTRY_H
#define CH_ASSET_REGISTRY_H

#include "engine/assets/asset.h"
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <string>

namespace CHEngine
{
    // Manages the cache of loaded assets and handle-to-path mappings.
    class AssetRegistry
    {
    public:
        AssetRegistry() = default;

        void Register(const std::string& resolvedPath, std::shared_ptr<Asset> asset);
        void Unregister(AssetHandle handle);
        void Clear();

        std::shared_ptr<Asset> Get(AssetHandle handle) const;
        std::shared_ptr<Asset> Get(const std::string& resolvedPath) const;
        
        bool Contains(AssetHandle handle) const;
        bool Contains(const std::string& resolvedPath) const;

        AssetHandle GetHandle(const std::string& resolvedPath) const;

    private:
        std::unordered_map<AssetHandle, std::shared_ptr<Asset>> m_AssetCache;
        std::unordered_map<std::string, AssetHandle> m_PathToHandle;
        mutable std::shared_mutex m_Mutex;
    };
}

#endif // CH_ASSET_REGISTRY_H
