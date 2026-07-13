#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/assets/loaders/asset_loader.h"
#include "engine/assets/pak_archive.h"
#include "engine/core/engine_module.h"
#include <filesystem>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Chained
{

class AssetManager : public EngineModule
{
public:
    AssetManager();
    ~AssetManager();

    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual void Update(Timestep ts) override;

    // Registers the loader for a specific asset type.
    void RegisterLoader(AssetType type, AssetLoader loader);

    void SetAssetDirectory(const std::filesystem::path& path) { m_AssetDirectory = path; }
    void SetProjectDirectory(const std::filesystem::path& path) { m_ProjectDirectory = path; }
    void SetEngineRoot(const std::filesystem::path& path) { m_EngineRoot = path; }

    [[nodiscard]] const std::filesystem::path& GetAssetDirectory() const { return m_AssetDirectory; }
    [[nodiscard]] const std::filesystem::path& GetProjectDirectory() const { return m_ProjectDirectory; }
    [[nodiscard]] const std::filesystem::path& GetEngineRoot() const { return m_EngineRoot; }

    // Resolves a path through the project root and caches the resolved value.
    [[nodiscard]] std::string ResolvePath(const std::string& path) const;
    // Resolves a path to an already-loaded asset handle, or 0 when missing.
    AssetHandle ResolveToHandle(const std::string& path) const;

    // Mounts a read-only .pak archive as an asset source, layered above the loose asset
    // directory (last-mounted-wins). Used by exported/packaged builds — the editor normally
    // never calls this and always reads loose files from disk. Returns false if the archive
    // could not be opened.
    bool MountPakArchive(const std::filesystem::path& pakPath, std::string_view mountName = "assets");

    // Get by path — returns a cached asset when available, otherwise loads it.
    template <typename T> std::shared_ptr<T> Get(const std::string& path)
    {
        AssetHandle handle = ResolveToHandle(path);
        if (handle != AssetHandle(0))
        {
            auto asset = GetAsset(handle, T::GetStaticType());
            if (asset)
            {
                return std::static_pointer_cast<T>(asset);
            }
        }
        // Not in cache — load it now
        return Load<T>(path);
    }

    template <typename T> std::shared_ptr<T> Get(AssetHandle handle)
    {
        return std::static_pointer_cast<T>(GetAsset(handle, T::GetStaticType()));
    }

    // Explicit load — creates, loads and caches the asset, returning nullptr on failure.
    template <typename T> std::shared_ptr<T> Load(const std::string& path)
    {
        return std::static_pointer_cast<T>(LoadAsset(path, T::GetStaticType()));
    }

    [[nodiscard]] size_t GetPendingFinalizeCount() const;
    [[nodiscard]] size_t GetLoadingAssetCount() const;
    [[nodiscard]] bool HasBackgroundWork() const;

    // Finalizes completed async loads and calls OnLoaded() within a small per-frame budget.
    void Update();

    // Interval (seconds) between .chasset staleness checks. 0 = disabled.
    void SetHotReloadInterval(float seconds) { m_HotReloadInterval = seconds; }
    float GetHotReloadInterval() const { return m_HotReloadInterval; }

private:
    void ReloadAsset(AssetHandle handle, AssetType type);
    void CheckModelHotReload();
    // If a .pak is mounted and contains `internalPath`, extracts it to an on-disk cache file
    // (once) and returns the cache path — loaders keep reading plain files unmodified.
    // Returns an empty string if no pak is mounted or the path isn't in it.
    std::string ExtractFromPak(const std::string& internalPath) const;

public:
    template <typename T> void Reload(const std::string& path)
    {
        AssetHandle handle = ResolveToHandle(path);
        ReloadAsset(handle, T::GetStaticType());
    }


    std::shared_ptr<Asset> GetAsset(AssetHandle handle, AssetType type);
    std::shared_ptr<Asset> LoadAsset(const std::string& path, AssetType type);
public:

    // Asset cache.
    std::unordered_map<AssetHandle, std::shared_ptr<Asset>> m_AssetCache;
    std::unordered_map<AssetType, AssetLoader> m_Loaders;

    // Path-to-handle mapping for quick lookup and a path resolution cache.
    mutable std::unordered_map<std::string, AssetHandle> m_PathToHandle;
    mutable std::unordered_map<std::string, std::string> m_PathCache;

    // Async loading support.
    std::deque<std::shared_ptr<Asset>> m_PendingAssets;
    mutable std::mutex m_PendingMutex;
    mutable std::recursive_mutex m_AssetLock;

    std::filesystem::path m_AssetDirectory;
    std::filesystem::path m_ProjectDirectory;
    std::filesystem::path m_EngineRoot;

    // Non-null once MountPakArchive() succeeds at least once. Left null in the editor.
    std::unique_ptr<PakArchiveManager> m_PakArchives;

    float m_HotReloadInterval = 3.0f;
    float m_HotReloadAccumulator = 0.0f;
};
} // namespace Chained

#endif // CH_ASSET_MANAGER_H