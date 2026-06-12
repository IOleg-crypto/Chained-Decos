#ifndef CH_ASSET_REGISTRY_H
#define CH_ASSET_REGISTRY_H

#include "engine/assets/asset_metadata.h"
#include <unordered_map>
#include <shared_mutex>

namespace Chained
{
    class AssetRegistry
    {
    public:
        AssetRegistry() = default;

        const AssetMetadata& GetMetadata(AssetHandle handle) const;
        void SetMetadata(AssetHandle handle, const AssetMetadata& metadata);
        bool Contains(AssetHandle handle) const;
        void Remove(AssetHandle handle);
        void Clear();

        const std::unordered_map<AssetHandle, AssetMetadata>& GetRegistryMap() const { return m_Registry; }

    private:
        std::unordered_map<AssetHandle, AssetMetadata> m_Registry;
        mutable std::shared_mutex m_RegistryLock;
    };
}

#endif // CH_ASSET_REGISTRY_H
