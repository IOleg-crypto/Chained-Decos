#include "engine/assets/asset_manager.h"
#include "engine/graphics/loaders/model_loader.h"
#include "engine/graphics/loaders/font_loader.h"
#include "engine/core/thread_pool.h"
#include "engine/core/profiler.h"
#include <chrono>

namespace CHEngine
{
    namespace
    {
        constexpr size_t kMaxAssetFinalizationsPerFrame = 16;
        constexpr auto kMaxAssetFinalizeBudget = std::chrono::milliseconds(2);
    }

    AssetManager::AssetManager(std::shared_ptr<AssetPathResolver> resolver, 
                               std::shared_ptr<AssetRegistry> registry)
        : m_PathResolver(std::move(resolver)), 
          m_Registry(std::move(registry))
    {
    }

    void AssetManager::RegisterLoader(AssetType type, std::shared_ptr<IAssetLoader> loader)
    {
        std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
        m_Loaders[type] = std::move(loader);
    }

    size_t AssetManager::GetPendingFinalizeCount() const
    {
        std::lock_guard<std::mutex> lock(m_PendingMutex);
        return m_PendingAssets.size();
    }

    size_t AssetManager::GetLoadingAssetCount() const
    {
        return 0; 
    }

    bool AssetManager::HasBackgroundWork() const
    {
        return GetPendingFinalizeCount() > 0;
    }

    std::shared_ptr<Asset> AssetManager::LoadAsset(const std::string& path, AssetType type)
    {
        if (path.empty()) return nullptr;

        std::string resolved = m_PathResolver->Resolve(path);

        std::shared_ptr<Asset> asset = m_Registry->Get(resolved);
        if (asset)
        {
            if (asset->GetType() != type && type != AssetType::None)
            {
                CH_CORE_ERROR("AssetManager: Type mismatch for '{}'. Expected {}, but found {}.", 
                              resolved, (int)type, (int)asset->GetType());
                return nullptr;
            }
            if (asset->GetState() != AssetState::None && asset->GetState() != AssetState::Failed)
            {
                return asset;
            }
        }

        std::shared_ptr<IAssetLoader> loader;
        {
            std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
            auto it = m_Loaders.find(type);
            if (it == m_Loaders.end() && type != AssetType::None)
            {
                CH_CORE_ERROR("AssetManager: No loader registered for type {}", (int)type);
                return nullptr;
            }
            if (it != m_Loaders.end())
            {
                loader = it->second;
            }
        }

        LoadContext ctx;
        ctx.ResolvedPath = resolved;
        
        uint64_t deterministicHash = 14695981039346656037ULL;
        for (char c : resolved) {
            deterministicHash ^= static_cast<uint64_t>(c);
            deterministicHash *= 1099511628211ULL;
        }
        ctx.Handle = asset ? asset->GetID() : AssetHandle(deterministicHash); 

        if (!asset)
        {
            asset = (loader) ? loader->Create() : std::make_shared<Asset>(type, ctx.Handle);
            asset->OverrideID(ctx.Handle); // Ensure deterministic ID even if loader->Create() made a random one
            asset->SetPath(resolved);
            m_Registry->Register(resolved, asset);
        }

        asset->SetState(AssetState::Loading);

        if (loader && !loader->IsAsync())
        {
            std::string error;
            if (loader->Load(asset, ctx, &error))
            {
                asset->SetState(AssetState::Ready);
                asset->OnLoaded();
                CH_CORE_INFO("[ASSET] '{}' loaded successfully (Sync)", resolved);
            }
            else
            {
                asset->SetState(AssetState::Failed);
                CH_CORE_ERROR("AssetManager: Failed to load '{}': {}", resolved, error);
            }
            return asset;
        }
        else if (loader)
        {
            ThreadPool::Get().QueueTask([this, asset, ctx, loader, resolved]() {
                std::string error;
                if (loader->Load(asset, ctx, &error))
                {
                    std::lock_guard<std::mutex> lock(m_PendingMutex);
                    m_PendingAssets.push_back(asset);
                }
                else
                {
                    asset->SetState(AssetState::Failed);
                    CH_CORE_ERROR("AssetManager: Async load failed for '{}': {}", resolved, error);
                }
            });

            return asset;
        }
        else
        {
            asset->SetState(AssetState::Ready);
            return asset;
        }
    }

    void AssetManager::OnUpdate(Timestep ts)
    {
        CH_PROFILE_FUNCTION();
        
        auto startTime = std::chrono::steady_clock::now();
        const auto budgetEnd = startTime + kMaxAssetFinalizeBudget;
        
        while (true)
        {
            std::shared_ptr<Asset> asset;
            {
                std::lock_guard<std::mutex> lock(m_PendingMutex);
                if (m_PendingAssets.empty()) break;
                asset = m_PendingAssets.front();
            }

            if (!asset) break;

            bool done = true;
            if (auto modelAsset = std::dynamic_pointer_cast<ModelAsset>(asset))
            {
                done = ModelLoader::Finalize(modelAsset, budgetEnd);
            }
            else if (auto fontAsset = std::dynamic_pointer_cast<FontAsset>(asset))
            {
                asset->OnLoaded();
            }
            else
            {
                // Fallback for types without specialized progressive finalization
                asset->OnLoaded();
            }

            if (done)
            {
                {
                    std::lock_guard<std::mutex> lock(m_PendingMutex);
                    m_PendingAssets.pop_front();
                }
                
                asset->SetState(AssetState::Ready);
                
                auto now = std::chrono::steady_clock::now();
                auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - asset->GetStartTime());
                
                CH_CORE_INFO("[ASSET] Ready: '{}' (Total: {}ms)", 
                             asset->GetPath(), totalDuration.count());
            }

            // Budget check: if we're not DONE or if we spent too much time already, stop.
            if (!done || std::chrono::steady_clock::now() >= budgetEnd)
            {
                break;
            }
        }
    }

    void AssetManager::OnShutdown()
    {
        CH_CORE_INFO("[ASSET] Shutting down Asset Manager...");
        // Clear registries if needed, though they are managed by smart pointers
    }

    void AssetManager::ReloadAsset(AssetHandle handle, AssetType type)
    {
    }

    AssetHandle AssetManager::ResolveToHandle(const std::string& path, AssetType type)
    {
        if (path.empty()) return AssetHandle(0);
        
        std::string resolved = m_PathResolver->Resolve(path);
        AssetHandle handle = m_Registry->GetHandle(resolved);
        if (handle != AssetHandle(0))
        {
            return handle;
        }

        auto asset = LoadAsset(path, type);
        if (asset) return asset->GetID();

        return AssetHandle(0);
    }

    std::string AssetManager::ResolvePath(const std::string& path) const
    {
        return m_PathResolver->Resolve(path);
    }
}
