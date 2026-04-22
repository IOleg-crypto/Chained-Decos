#include "engine/core/assets/asset_manager.h"
#include "engine/core/thread_pool.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include <chrono>

namespace CHEngine
{
namespace
{
constexpr size_t kMaxAssetFinalizationsPerFrame = 16;
constexpr auto kMaxAssetFinalizeBudget = std::chrono::milliseconds(2);
}

AssetManager::AssetManager()
{
}

AssetManager::~AssetManager()
{
    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
    m_AssetCache.clear();
    m_PathToHandle.clear();
    m_PathCache.clear();
    m_Loaders.clear();
}

AssetManager& AssetManager::Get()
{
    return ServiceLocator::Get<AssetManager>();
}

void AssetManager::RegisterLoader(AssetType type, std::unique_ptr<IAssetLoader> loader)
{
    m_Loaders[type] = std::move(loader);
}

std::string AssetManager::ResolvePath(const std::string& path) const
{
    if (path.empty())
    {
        return "";
    }

    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (auto it = m_PathCache.find(path); it != m_PathCache.end())
        {
            return it->second;
        }
    }

    std::filesystem::path inputPath(path);
    std::filesystem::path resolvedPath;

    if (inputPath.is_absolute())
    {
        resolvedPath = inputPath;
    }
    else
    {
        std::string pathStr = inputPath.generic_string();
        bool isEngineResource = false;
        if (pathStr.find("engine/") == 0)
        {
            pathStr = pathStr.substr(7);
            isEngineResource = true;
        }

        if (isEngineResource)
        {
            if (!m_EngineRoot.empty())
            {
                std::filesystem::path candidate = m_EngineRoot / pathStr;
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

        // Fallback or Fuzzy Search
        if (resolvedPath.empty())
        {
            // Try fuzzy search for moved assets (search for filename in asset directory)
            std::string filename = inputPath.filename().string();
            if (!filename.empty() && !m_AssetDirectory.empty())
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(m_AssetDirectory))
                {
                    if (entry.is_regular_file() && entry.path().filename() == filename)
                    {
                        resolvedPath = entry.path();
                        CH_CORE_WARN("AssetManager: Asset '{}' not found at original path, but found at '{}'. Please update references.", 
                                     path, resolvedPath.generic_string());
                        break;
                    }
                }
            }

            if (resolvedPath.empty())
            {
                if (!isEngineResource && !m_AssetDirectory.empty())
                {
                    resolvedPath = m_AssetDirectory / pathStr;
                }
                else if (!m_EngineRoot.empty())
                {
                    resolvedPath = m_EngineRoot / pathStr;
                }
                else
                {
                    resolvedPath = m_ProjectDirectory / pathStr;
                }
            }
        }
    }

    // Normalize and convert to string
    std::string resolved = std::filesystem::absolute(resolvedPath).lexically_normal().generic_string();

    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
    m_PathCache[path] = resolved;
    return resolved;
}

AssetHandle AssetManager::ResolveToHandle(const std::string& path) const
{
    if (path.empty()) return AssetHandle(0);

    // First, check the path cache to see if we've already resolved this input path
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        auto it = m_PathCache.find(path);
        if (it != m_PathCache.end())
        {
            auto handleIt = m_PathToHandle.find(it->second);
            if (handleIt != m_PathToHandle.end())
                return handleIt->second;
        }
    }

    // Not in cache, do full resolution
    std::string resolved = ResolvePath(path);
    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
    if (auto it = m_PathToHandle.find(resolved); it != m_PathToHandle.end())
    {
        return it->second;
    }
    return AssetHandle(0);
}

std::shared_ptr<Asset> AssetManager::LoadAsset(const std::string& path, AssetType type)
{
    if (path.empty())
    {
        return nullptr;
    }

    std::string resolved = ResolvePath(path);

    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (auto it = m_PathToHandle.find(resolved); it != m_PathToHandle.end())
        {
            auto handle = it->second;
            if (auto currentIt = m_AssetCache.find(handle); currentIt != m_AssetCache.end())
            {
                auto asset = currentIt->second;
                if (asset && asset->GetType() != type)
                {
                    CH_CORE_ERROR("AssetManager: Type mismatch for '{}'. Expected {}, but found {}.", 
                                  resolved, (int)type, (int)asset->GetType());
                    return nullptr;
                }
                return asset;
            }
        }
    }

    IAssetLoader* loader = nullptr;
    std::shared_ptr<Asset> asset;

    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        auto loaderIt = m_Loaders.find(type);
        if (loaderIt == m_Loaders.end())
        {
            CH_CORE_ERROR("AssetManager: No loader registered for type {}", (int)type);
            return nullptr;
        }

        asset = loaderIt->second->Create();
        if (!asset)
        {
            return nullptr;
        }

        loader = loaderIt->second.get();
        AssetHandle newHandle = asset->GetID();
        asset->SetPath(resolved);
        asset->SetState(AssetState::Loading);
        asset->ClearError();
        m_AssetCache[newHandle] = asset;
        m_PathToHandle[resolved] = newHandle;
    }

    if (!loader->IsAsync())
    {
        try
        {
            std::string loaderError;
            if (!loader->Load(asset, resolved, &loaderError))
            {
                asset->Fail(loaderError.empty() ? ("AssetManager: Synchronous loader returned false for '" + resolved + "'")
                                                : loaderError);
                return asset;
            }

            asset->ClearError();
            asset->OnLoaded();
            asset->SetState(AssetState::Ready);
        }
        catch (const std::exception& e)
        {
            asset->Fail(std::string("AssetManager: Synchronous load failed for '") + resolved + "': " + e.what());
        }
        catch (...)
        {
            asset->Fail(std::string("AssetManager: Synchronous load failed for '") + resolved + "' with an unknown exception");
        }
    }
    else
    {
        ThreadPool::Get().QueueTask([this, asset, loader, resolved]() {
            try
            {
                std::string loaderError;
                if (!loader->Load(asset, resolved, &loaderError))
                {
                    asset->Fail(loaderError.empty() ? ("AssetManager: Async loader returned false for '" + resolved + "'")
                                                    : loaderError);
                    return;
                }

                asset->ClearError();
                std::lock_guard<std::mutex> lock(m_PendingMutex);
                m_PendingAssets.push_back(asset);
            }
            catch (const std::exception& e)
            {
                asset->Fail(std::string("AssetManager: Async load failed for '") + resolved + "': " + e.what());
            }
            catch (...)
            {
                asset->Fail(std::string("AssetManager: Async load failed for '") + resolved + "' with an unknown exception");
            }
        });
    }

    return asset;
}

std::shared_ptr<Asset> AssetManager::GetAsset(AssetHandle handle, AssetType type)
{
    if (handle != AssetHandle(0))
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (auto it = m_AssetCache.find(handle); it != m_AssetCache.end())
        {
            auto asset = it->second;
            if (asset && asset->GetType() != type && type != AssetType::None)
            {
                CH_CORE_ERROR("AssetManager: Type mismatch for handle {}. Expected {}, but found {}.", 
                              (uint64_t)handle, (int)type, (int)asset->GetType());
                return nullptr;
            }
            return asset;
        }
    }

    return nullptr;
}

void AssetManager::Update()
{
    CH_PROFILE_FUNCTION();

    const auto updateStart = std::chrono::steady_clock::now();
    size_t finalizedCount = 0;

    while (finalizedCount < kMaxAssetFinalizationsPerFrame)
    {
        std::shared_ptr<Asset> asset;
        {
            std::lock_guard<std::mutex> lock(m_PendingMutex);
            if (m_PendingAssets.empty())
            {
                return;
            }

            asset = std::move(m_PendingAssets.front());
            m_PendingAssets.pop_front();
        }

        if (!asset)
        {
            continue;
        }

        try
        {
            asset->ClearError();
            asset->OnLoaded();
            if (asset->GetState() != AssetState::Failed)
            {
                asset->SetState(AssetState::Ready);
            }
        }
        catch (const std::exception& e)
        {
            asset->Fail(std::string("AssetManager: Finalization failed for '") + asset->GetPath() + "': " + e.what());
        }
        catch (...)
        {
            asset->Fail(std::string("AssetManager: Finalization failed for '") + asset->GetPath() + "' with an unknown exception");
        }

        ++finalizedCount;

        if ((std::chrono::steady_clock::now() - updateStart) >= kMaxAssetFinalizeBudget)
        {
            break;
        }
    }
}

size_t AssetManager::GetPendingFinalizeCount() const
{
    std::lock_guard<std::mutex> lock(m_PendingMutex);
    return m_PendingAssets.size();
}

size_t AssetManager::GetLoadingAssetCount() const
{
    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);

    size_t loadingCount = 0;
    for (const auto& [handle, asset] : m_AssetCache)
    {
        (void)handle;
        if (asset && asset->GetState() == AssetState::Loading)
        {
            ++loadingCount;
        }
    }

    return loadingCount;
}

bool AssetManager::HasBackgroundWork() const
{
    return GetPendingFinalizeCount() > 0 || GetLoadingAssetCount() > 0;
}

void AssetManager::ReloadAsset(AssetHandle handle, AssetType type)
{
    std::shared_ptr<Asset> asset;
    IAssetLoader* loader = nullptr;
    std::string path;

    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);

        auto it = m_AssetCache.find(handle);
        if (it == m_AssetCache.end())
        {
            return;
        }

        auto loaderIt = m_Loaders.find(type);
        if (loaderIt == m_Loaders.end())
        {
            return;
        }

        asset = it->second;
        loader = loaderIt->second.get();
        path = asset->GetPath();
        if (path.empty())
        {
            return;
        }
    }

    std::string resolved = ResolvePath(path);
    asset->SetState(AssetState::Loading);
    asset->ClearError();

    try
    {
        std::string loaderError;
        if (!loader->Load(asset, resolved, &loaderError))
        {
            asset->Fail(loaderError.empty() ? ("AssetManager: Reload failed for '" + resolved + "'") : loaderError);
            return;
        }

        asset->ClearError();
        asset->OnLoaded();
        asset->SetState(AssetState::Ready);
    }
    catch (const std::exception& e)
    {
        asset->Fail(std::string("AssetManager: Reload failed for '") + resolved + "': " + e.what());
    }
    catch (...)
    {
        asset->Fail(std::string("AssetManager: Reload failed for '") + resolved + "' with an unknown exception");
    }
}

} // namespace CHEngine
