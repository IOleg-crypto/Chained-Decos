#include "engine/assets/asset_path_resolver.h"

#include <filesystem>
#include "engine/core/log.h"

namespace CHEngine
{
    void AssetPathResolver::SetRoots(const std::filesystem::path& engineRoot,
                                     const std::filesystem::path& projectDirectory,
                                     const std::filesystem::path& assetDirectory)
    {
        std::unique_lock lock(m_CacheMutex);
        m_EngineRoot = engineRoot;
        m_ProjectDirectory = projectDirectory;
        m_AssetDirectory = assetDirectory;
        m_PathCache.clear();
    }

    void AssetPathResolver::SetEngineRoot(const std::filesystem::path& path)
    {
        std::unique_lock lock(m_CacheMutex);
        m_EngineRoot = path;
        m_PathCache.clear();
    }

    void AssetPathResolver::SetProjectDirectory(const std::filesystem::path& path)
    {
        std::unique_lock lock(m_CacheMutex);
        m_ProjectDirectory = path;
        m_PathCache.clear();
    }

    void AssetPathResolver::SetAssetDirectory(const std::filesystem::path& path)
    {
        std::unique_lock lock(m_CacheMutex);
        m_AssetDirectory = path;
        m_PathCache.clear();
    }

    void AssetPathResolver::ClearCache()
    {
        std::unique_lock lock(m_CacheMutex);
        m_PathCache.clear();
    }

    std::string AssetPathResolver::Resolve(const std::string& path) const
    {
        if (path.empty()) return "";

        {
            std::shared_lock lock(m_CacheMutex);
            if (auto it = m_PathCache.find(path); it != m_PathCache.end())
            {
                return it->second;
            }
        }

        std::string resolved = InternalResolve(path);

        {
            std::unique_lock lock(m_CacheMutex);
            m_PathCache[path] = resolved;
        }

        return resolved;
    }

    std::string AssetPathResolver::InternalResolve(const std::string& path) const
    {
        std::filesystem::path inputPath(path);
        if (inputPath.is_absolute())
        {
            return std::filesystem::absolute(inputPath).lexically_normal().generic_string();
        }

        std::string pathStr = inputPath.generic_string();
        std::filesystem::path resolvedPath;

        bool isEngineResource = (pathStr.find("engine/") == 0);
        if (isEngineResource)
        {
            std::string relative = pathStr.substr(7);
            if (!m_EngineRoot.empty())
            {
                std::filesystem::path candidate = m_EngineRoot / relative;
                if (std::filesystem::exists(candidate))
                {
                    resolvedPath = candidate;
                }
            }
        }
        else
        {
            // Try asset directory first
            if (!m_AssetDirectory.empty())
            {
                std::filesystem::path candidate = m_AssetDirectory / pathStr;
                if (std::filesystem::exists(candidate))
                {
                    resolvedPath = candidate;
                }
            }

            // Try project root next
            if (resolvedPath.empty() && !m_ProjectDirectory.empty())
            {
                std::filesystem::path candidate = m_ProjectDirectory / pathStr;
                if (std::filesystem::exists(candidate))
                {
                    resolvedPath = candidate;
                }
            }
        }

        // Fuzzy search fallback
        if (resolvedPath.empty())
        {
            std::string filename = inputPath.filename().string();
            if (!filename.empty() && !m_AssetDirectory.empty())
            {
                std::error_code ec;
                if (std::filesystem::exists(m_AssetDirectory, ec) && std::filesystem::is_directory(m_AssetDirectory, ec))
                {
                    auto it = std::filesystem::recursive_directory_iterator(m_AssetDirectory, std::filesystem::directory_options::skip_permission_denied, ec);
                    auto end = std::filesystem::recursive_directory_iterator();
                    
                    if (!ec)
                    {
                        for (; it != end; it.increment(ec))
                        {
                            if (ec) break;
                            const auto& entry = *it;
                            if (entry.is_regular_file(ec) && entry.path().filename() == filename)
                            {
                                resolvedPath = entry.path();
                                CH_CORE_INFO("AssetPathResolver: Asset '{}' found at fuzzy path '{}'", path, resolvedPath.generic_string());
                                break;
                            }
                        }
                    }
                }
            }

            if (resolvedPath.empty())
            {
                // Absolute fallback to Projekt root or Engine root
                if (isEngineResource && !m_EngineRoot.empty())
                    resolvedPath = m_EngineRoot / pathStr.substr(7);
                else if (!m_AssetDirectory.empty())
                    resolvedPath = m_AssetDirectory / pathStr;
                else if (!m_ProjectDirectory.empty())
                    resolvedPath = m_ProjectDirectory / pathStr;
                else
                    resolvedPath = inputPath;
            }
        }

        std::error_code ec;
        auto abs = std::filesystem::absolute(resolvedPath, ec);
        if (ec) return resolvedPath.generic_string();
        
        return abs.lexically_normal().generic_string();
    }
}
