#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "engine/core/base.h"
#include "engine/graphics/asset.h"
#include <map>
#include <memory>
#include <string>
#include <filesystem>
#include <mutex>
#include <unordered_map>

namespace CHEngine
{
    struct AssetMetadata
    {
        AssetHandle Handle;
        std::string FilePath;
        AssetType Type;
    };

    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        void Initialize(const std::filesystem::path& rootPath = "");
        void Shutdown();

        void SetRootPath(const std::filesystem::path& path);
        std::filesystem::path GetRootPath() const;
        void AddSearchPath(const std::filesystem::path& path);
        void ClearSearchPaths();

        std::string ResolvePath(const std::string& path) const;

        template <typename T>
        std::shared_ptr<T> Get(const std::string& path)
        {
            return std::static_pointer_cast<T>(GetAsset(path, T::GetStaticType()));
        }

        template <typename T>
        std::shared_ptr<T> Get(AssetHandle handle)
        {
            return std::static_pointer_cast<T>(GetAsset(handle, T::GetStaticType()));
        }

        const AssetMetadata& GetMetadata(AssetHandle handle) const;
        void Update();

        template <typename T>
        void Remove(const std::string& path)
        {
            RemoveAsset(path, T::GetStaticType());
        }

    private:
        // Internal methods to be implemented in .cpp
        std::shared_ptr<Asset> GetAsset(const std::string& path, AssetType type);
        std::shared_ptr<Asset> GetAsset(AssetHandle handle, AssetType type);
        void RemoveAsset(const std::string& path, AssetType type);

    private:
        std::filesystem::path m_RootPath;
        std::vector<std::filesystem::path> m_SearchPaths;
        mutable std::recursive_mutex m_AssetLock;

        // Unified cache: Type -> Path -> Asset
        std::map<AssetType, std::map<std::string, std::shared_ptr<Asset>>> m_AssetCaches;
        std::unordered_map<AssetHandle, AssetMetadata> m_AssetMetadata;
    };
}

#endif // CH_ASSET_MANAGER_H
