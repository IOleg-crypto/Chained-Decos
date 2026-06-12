#include "engine/assets/asset_registry.h"

namespace Chained
{
    static const AssetMetadata s_NullMetadata;

    const AssetMetadata& AssetRegistry::GetMetadata(AssetHandle handle) const
    {
        std::shared_lock<std::shared_mutex> lock(m_RegistryLock);
        auto it = m_Registry.find(handle);
        if (it != m_Registry.end())
        {
            return it->second;
        }
        return s_NullMetadata;
    }

    void AssetRegistry::SetMetadata(AssetHandle handle, const AssetMetadata& metadata)
    {
        std::unique_lock<std::shared_mutex> lock(m_RegistryLock);
        m_Registry[handle] = metadata;
    }

    bool AssetRegistry::Contains(AssetHandle handle) const
    {
        std::shared_lock<std::shared_mutex> lock(m_RegistryLock);
        return m_Registry.find(handle) != m_Registry.end();
    }

    void AssetRegistry::Remove(AssetHandle handle)
    {
        std::unique_lock<std::shared_mutex> lock(m_RegistryLock);
        m_Registry.erase(handle);
    }

    void AssetRegistry::Clear()
    {
        std::unique_lock<std::shared_mutex> lock(m_RegistryLock);
        m_Registry.clear();
    }
}
