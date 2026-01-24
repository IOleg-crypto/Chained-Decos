#include "asset_manager.h"
#include "engine/core/log.h"
#include "engine/core/main_thread_queue.h"
#include "engine/scene/project.h"
#include <filesystem>
#include <fstream>
#include <future>

namespace CHEngine
{

std::unordered_map<std::string, Ref<Asset>, Assets::StringHash, std::equal_to<>> Assets::s_Assets;
std::mutex Assets::s_AssetsMutex;

void Assets::Init()
{
    CH_CORE_INFO("Assets System Initialized (Unified)");
}

void Assets::Shutdown()
{
    std::lock_guard<std::mutex> lock(s_AssetsMutex);
    s_Assets.clear();
    CH_CORE_INFO("Assets System Shut Down");
}

Ref<ShaderAsset> Assets::LoadShader(const std::string &vsPath, const std::string &fsPath)
{
    std::string key = vsPath + "|" + fsPath;
    {
        std::lock_guard<std::mutex> lock(s_AssetsMutex);
        if (s_Assets.count(key))
            return std::static_pointer_cast<ShaderAsset>(s_Assets[key]);
    }

    auto asset = ShaderAsset::Load(vsPath, fsPath);
    if (asset)
    {
        std::lock_guard<std::mutex> lock(s_AssetsMutex);
        s_Assets[key] = asset;
    }
    return asset;
}

Ref<ShaderAsset> Assets::LoadShader(const std::string &path)
{
    return Get<ShaderAsset>(path);
}

Ref<EnvironmentAsset> Assets::LoadEnvironment(const std::string &path)
{
    return Get<EnvironmentAsset>(path);
}

std::future<Ref<ModelAsset>> Assets::LoadModelAsync(const std::string &path)
{
    {
        std::lock_guard<std::mutex> lock(s_AssetsMutex);
        if (s_Assets.count(path))
        {
            std::promise<Ref<ModelAsset>> promise;
            promise.set_value(std::static_pointer_cast<ModelAsset>(s_Assets[path]));
            return promise.get_future();
        }
    }

    auto promise = std::make_shared<std::promise<Ref<ModelAsset>>>();
    std::future<Ref<ModelAsset>> future = promise->get_future();

    // Use std::async instead of TaskSystem
    std::async(std::launch::async,
               [path, promise]()
               {
                   // Background work
                   auto fullPath = ResolvePath(path);
                   if (std::filesystem::exists(fullPath))
                   {
                       std::ifstream f(fullPath, std::ios::binary | std::ios::ate);
                   }

                   // Finalize on Main Thread
                   MainThread::Execute(
                       [path, promise]()
                       {
                           Ref<ModelAsset> asset = Assets::Get<ModelAsset>(path);
                           promise->set_value(asset);
                       });
               });

    return future;
}

std::filesystem::path Assets::ResolvePath(const std::string &path)
{
    if (path.empty())
        return "";

    if (path.starts_with("engine:"))
        return (std::filesystem::path(PROJECT_ROOT_DIR) / "engine/resources" / path.substr(7))
            .make_preferred();

    std::filesystem::path p = path;
    if (p.is_absolute())
        return p.make_preferred();

    if (Project::GetActive())
    {
        std::filesystem::path assetDir = Project::GetAssetDirectory();

        // Try relative to asset directory
        std::filesystem::path fullPath = (assetDir / p).make_preferred();
        if (std::filesystem::exists(fullPath))
            return fullPath;

        // Try stripping "assets/" if it's double-prefixed
        std::string pathStr = p.string();
        if (pathStr.starts_with("assets/") || pathStr.starts_with("assets\\"))
        {
            std::filesystem::path stripped = pathStr.substr(7);
            fullPath = (assetDir / stripped).make_preferred();
            if (std::filesystem::exists(fullPath))
                return fullPath;

            // Also try relative to project root directly if it's already "assets/..."
            fullPath = (Project::GetProjectDirectory() / p).make_preferred();
            if (std::filesystem::exists(fullPath))
                return fullPath;
        }

        // Default to asset directory for new files
        return (assetDir / p).make_preferred();
    }

    return p.make_preferred();
}

} // namespace CHEngine
