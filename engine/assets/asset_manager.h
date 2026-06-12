#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/assets/asset.h"
#include "engine/assets/asset_registry.h"
#include "engine/foundation/timestep.h"
#include "engine/foundation/uuid.h"

#include <filesystem>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <string_view>

namespace Chained
{
class AssetManager
{
public:
    static constexpr std::string_view ProjectExtension = ".chproject";
    static constexpr std::string_view SceneExtension = ".chscene";
    static constexpr std::string_view PrefabExtension = ".chprefab";

public:
    static void Init();
    static void Shutdown();
    static AssetManager& Get();

    void SetEngineRoot(const std::filesystem::path& path);
    void SetProjectDirectory(const std::filesystem::path& path);
    void SetAssetDirectory(const std::filesystem::path& path);

    const std::filesystem::path& GetAssetDirectory() const { return m_AssetDirectory; }
    const std::filesystem::path& GetProjectDirectory() const { return m_ProjectDirectory; }
    const std::filesystem::path& GetEngineRoot() const { return m_EngineRoot; }

    const AssetMetadata& GetMetadata(AssetHandle handle) const;
    void SetMetadata(AssetHandle handle, const AssetMetadata& metadata);
    const AssetRegistry& GetRegistry() const { return m_Registry; }

    AssetHandle ImportAsset(const std::filesystem::path& filepath);
    
    std::shared_ptr<Asset> GetAssetRaw(AssetHandle handle);

    template <typename T>
    std::shared_ptr<T> GetAsset(AssetHandle handle)
    {
        auto asset = GetAssetRaw(handle);
        return std::static_pointer_cast<T>(asset);
    }

    void Update(Timestep ts);

    bool HasBackgroundWork() const { return false; }
    uint32_t GetPendingFinalizeCount() const { return 0; }
    AssetHandle ResolveToHandle(const std::filesystem::path& path, AssetType type = AssetType::None);

private:
    std::unordered_map<AssetHandle, std::shared_ptr<Asset>> m_LoadedAssets;
    AssetRegistry m_Registry;
    mutable std::mutex m_AssetMutex;

    std::filesystem::path m_EngineRoot;
    std::filesystem::path m_ProjectDirectory;
    std::filesystem::path m_AssetDirectory;

    static AssetManager* s_Instance;
};
}

#endif // CH_ASSET_MANAGER_H