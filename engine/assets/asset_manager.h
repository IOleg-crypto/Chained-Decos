#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/assets/loaders/iasset_loader.h"
#include "engine/core/service.h"
#include "engine/common/timestep.h"
#include <filesystem>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pack
{
class Reader;
}

namespace Chained
{

class AssetManager : public Service
{
public:
    AssetManager();
    ~AssetManager();
    virtual void Initialize();
    virtual void Shutdown();
    void Update(Timestep ts);

    void RegisterLoader(AssetType type, std::unique_ptr<IAssetLoader> loader);

    void SetAssetDirectory(const std::filesystem::path& path)
    {
        m_AssetDirectory = path;
    }
    void SetProjectDirectory(const std::filesystem::path& path)
    {
        m_ProjectDirectory = path;
    }
    void SetEngineRoot(const std::filesystem::path& path)
    {
        m_EngineRoot = path;
    }

    bool OpenPack(const std::filesystem::path& packPath);
    bool IsPacked() const
    {
        return m_PackOpen;
    }
    std::vector<uint8_t> ReadAssetData(const std::string& assetPath);
    bool HasAsset(const std::string& path) const;
    void EnumeratePackedPaths(const std::function<void(std::string_view)>& callback) const;
    std::vector<uint8_t> ReadProjectAsset(const std::filesystem::path& absolutePath);

    const std::filesystem::path& GetAssetDirectory() const
    {
        return m_AssetDirectory;
    }
    const std::filesystem::path& GetProjectDirectory() const
    {
        return m_ProjectDirectory;
    }
    const std::filesystem::path& GetEngineRoot() const
    {
        return m_EngineRoot;
    }

    std::string ResolvePath(const std::string& path) const;
    AssetHandle ResolveToHandle(const std::string& path) const;

    template <typename T> std::shared_ptr<T> Get(const std::string& path)
    {
        AssetHandle handle = ResolveToHandle(path);
        if (handle != AssetHandle(0))
        {
            auto asset = GetAsset(handle);
            if (asset)
            {
                return std::static_pointer_cast<T>(asset);
            }
        }
        return Load<T>(path);
    }

    template <typename T> std::shared_ptr<T> Get(AssetHandle handle)
    {
        return std::static_pointer_cast<T>(GetAsset(handle));
    }

    template <typename T> std::shared_ptr<T> GetByUUID(uint64_t uuid)
    {
        return std::static_pointer_cast<T>(GetAsset(AssetHandle(uuid)));
    }

    template <typename T> std::shared_ptr<T> Load(const std::string& path)
    {
        return std::static_pointer_cast<T>(LoadAsset(path, T::GetStaticType()));
    }

    size_t GetPendingFinalizeCount() const;
    size_t GetLoadingAssetCount() const;
    bool HasBackgroundWork() const;

    void FinalizePendingLoads();

    void SetHotReloadInterval(float seconds)
    {
        m_HotReloadInterval = seconds;
    }
    float GetHotReloadInterval() const
    {
        return m_HotReloadInterval;
    }

    template <typename T> void Reload(const std::string& path)
    {
        AssetHandle handle = ResolveToHandle(path);
        ReloadAsset(handle, T::GetStaticType());
    }

    void Invalidate(const std::string& path, bool deleteFromDisk = false);
    bool DeleteChasset(const std::string& path);
    size_t DeleteAllChassets();

    std::vector<std::string> GetStaleAssets() const;
    size_t ReloadAllStale();

    std::shared_ptr<Asset> GetAsset(AssetHandle handle);
    std::shared_ptr<Asset> LoadAsset(const std::string& path, AssetType type);

private:
    struct StaleAsset
    {
        AssetHandle handle;
        AssetType type;
        std::string path;
    };

    void ReloadAsset(AssetHandle handle, AssetType type);
    void CheckAssetHotReload();
    std::vector<StaleAsset> CollectStaleAssets(int thresholdSeconds) const;
    bool ExecuteLoad(const std::shared_ptr<Asset>& asset, IAssetLoader* loader, const std::string& resolved);

    std::string ResolvePackKey(const std::string& assetPath) const;

    std::unordered_map<AssetHandle, std::shared_ptr<Asset>> m_AssetCache;
    std::unordered_map<AssetType, std::unique_ptr<IAssetLoader>> m_Loaders;

    mutable std::unordered_map<std::string, AssetHandle> m_PathToHandle;
    mutable std::unordered_map<std::string, std::string> m_PathCache;

    std::deque<std::shared_ptr<Asset>> m_PendingAssets;
    mutable std::mutex m_PendingMutex;
    mutable std::mutex m_AssetLock;

    std::filesystem::path m_AssetDirectory;
    std::filesystem::path m_ProjectDirectory;
    std::filesystem::path m_EngineRoot;

    std::unique_ptr<pack::Reader> m_PackReader;
    bool m_PackOpen = false;

    float m_HotReloadInterval = 3.0f;
    float m_HotReloadAccumulator = 0.0f;
};
} // namespace Chained

#endif // CH_ASSET_MANAGER_H
