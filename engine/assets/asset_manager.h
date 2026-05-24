#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/assets/asset_loader.h"
#include "engine/assets/asset_path_resolver.h"
#include "engine/assets/asset_registry.h"
#include "engine/core/engine_service.h"
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace CHEngine
{
class AssetManager : public EngineService
{
public:
    static constexpr std::string_view ProjectExtension = ".chproject";
    static constexpr std::string_view SceneExtension = ".chscene";
    static constexpr std::string_view PrefabExtension = ".chprefab";

public:
    AssetManager(std::shared_ptr<AssetPathResolver> resolver, std::shared_ptr<AssetRegistry> registry);
    virtual ~AssetManager() override = default;

    void RegisterLoader(AssetType type, std::shared_ptr<IAssetLoader> loader);

    std::shared_ptr<AssetPathResolver> GetResolver() const
    {
        return m_PathResolver;
    }
    std::shared_ptr<AssetRegistry> GetRegistry() const
    {
        return m_Registry;
    }
    template <typename T> std::shared_ptr<T> Get(AssetHandle handle)
    {
        auto asset = m_Registry->Get(handle);
        if (asset && asset->GetType() == T::GetStaticType())
        {
            if (asset->GetState() != AssetState::None)
            {
                return std::static_pointer_cast<T>(asset);
            }
            return std::static_pointer_cast<T>(LoadAsset(asset->GetPath(), T::GetStaticType()));
        }
        return nullptr;
    }

    template <typename T> void Reload(const std::string& path)
    {
        std::string resolved = m_PathResolver->Resolve(path);
        AssetHandle handle = m_Registry->GetHandle(resolved);
        if (handle != AssetHandle(0))
        {
            ReloadAsset(handle, T::GetStaticType());
        }
    }

    size_t GetPendingFinalizeCount() const;
    size_t GetLoadingAssetCount() const;
    bool HasBackgroundWork() const;

    AssetHandle ResolveToHandle(const std::string& path, AssetType type = AssetType::None);
    std::string ResolvePath(const std::string& path) const;

    std::filesystem::path GetAssetDirectory() const
    {
        return m_PathResolver->GetAssetDirectory();
    }
    std::filesystem::path GetProjectDirectory() const
    {
        return m_PathResolver->GetProjectDirectory();
    }
    std::filesystem::path GetEngineRoot() const
    {
        return m_PathResolver->GetEngineRoot();
    }

public:
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnShutdown() override;

protected:

private:
    void ReloadAsset(AssetHandle handle, AssetType type);
    std::shared_ptr<Asset> LoadAsset(const std::string& path, AssetType type);

    std::shared_ptr<AssetPathResolver> m_PathResolver;
    std::shared_ptr<AssetRegistry> m_Registry;

    std::unordered_map<AssetType, std::shared_ptr<IAssetLoader>> m_Loaders;

    std::deque<std::shared_ptr<Asset>> m_PendingAssets;
    mutable std::mutex m_PendingMutex;
    mutable std::recursive_mutex m_AssetLock;
};
} // namespace CHEngine

#endif // CH_ASSET_MANAGER_H
