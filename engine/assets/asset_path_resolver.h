#ifndef CH_ASSET_PATH_RESOLVER_H
#define CH_ASSET_PATH_RESOLVER_H

#include <filesystem>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace CHEngine
{
// Resonsible for resolving engine-relative and project-relative paths.
class AssetPathResolver
{
public:
    AssetPathResolver() = default;

    void SetRoots(const std::filesystem::path& engineRoot, const std::filesystem::path& projectDirectory,
                  const std::filesystem::path& assetDirectory);

    void SetEngineRoot(const std::filesystem::path& path);
    void SetProjectDirectory(const std::filesystem::path& path);
    void SetAssetDirectory(const std::filesystem::path& path);

    [[nodiscard]] const std::filesystem::path& GetEngineRoot() const
    {
        return m_EngineRoot;
    }
    [[nodiscard]] const std::filesystem::path& GetProjectDirectory() const
    {
        return m_ProjectDirectory;
    }
    [[nodiscard]] const std::filesystem::path& GetAssetDirectory() const
    {
        return m_AssetDirectory;
    }

    // Resolves a path (e.g. "engine/shaders/lit.chshader") to an absolute filesystem path.
    [[nodiscard]] std::string Resolve(const std::string& path) const;

    // Clears the resolution cache.
    void ClearCache();

private:
    [[nodiscard]] std::string InternalResolve(const std::string& path) const;

    std::filesystem::path m_EngineRoot;
    std::filesystem::path m_ProjectDirectory;
    std::filesystem::path m_AssetDirectory;

    mutable std::unordered_map<std::string, std::string> m_PathCache;
    mutable std::shared_mutex m_CacheMutex;
};
} // namespace CHEngine

#endif // CH_ASSET_PATH_RESOLVER_H
