#include "engine/core/assets/asset_manager.h"
#include "engine/core/thread_pool.h"
#include "engine/core/profiler.h"
#include "engine/scene/project.h"

namespace CHEngine
{
namespace
{
constexpr size_t kMaxAssetFinalizationsPerFrame = 4;
}

static std::unique_ptr<AssetManager> s_Instance = nullptr;

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
    if (!s_Instance)
    {
        s_Instance = std::make_unique<AssetManager>();
    }
    return *s_Instance;
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

    std::string resolved = Project::GetAbsolutePath(path).string();

    std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
    m_PathCache[path] = resolved;
    return resolved;
}

AssetHandle AssetManager::ResolveToHandle(const std::string& path) const
{
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
                return currentIt->second;
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
    (void)type;

    if (handle != 0)
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        if (auto it = m_AssetCache.find(handle); it != m_AssetCache.end())
        {
            return it->second;
        }
    }

    return nullptr;
}

void AssetManager::Update()
{
    CH_PROFILE_FUNCTION();

    std::vector<std::shared_ptr<Asset>> completed;
    {
        std::lock_guard<std::mutex> lock(m_PendingMutex);
        if (m_PendingAssets.empty())
        {
            return;
        }

        const size_t finalizeCount = (m_PendingAssets.size() < kMaxAssetFinalizationsPerFrame)
                                         ? m_PendingAssets.size()
                                         : kMaxAssetFinalizationsPerFrame;

        completed.reserve(finalizeCount);
        for (size_t finalizeIndex = 0; finalizeIndex < finalizeCount; ++finalizeIndex)
        {
            completed.push_back(std::move(m_PendingAssets[finalizeIndex]));
        }
        m_PendingAssets.erase(m_PendingAssets.begin(), m_PendingAssets.begin() + finalizeCount);
    }

    for (auto& asset : completed)
    {
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
