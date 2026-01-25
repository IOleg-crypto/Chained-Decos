#include "asset_manager.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include <filesystem>
#include <fstream>
#include <future>

namespace CHEngine
{

std::unordered_map<std::string, std::shared_ptr<Asset>, Assets::StringHash, std::equal_to<>>
    Assets::s_Assets;
std::mutex Assets::s_AssetsMutex;
std::vector<std::shared_ptr<Asset>> Assets::s_GPUUploadQueue;
std::mutex Assets::s_GPUQueueMutex;

void Assets::Init()
{
    CH_CORE_INFO("Assets System Initialized (Unified)");
}

void Assets::Shutdown()
{
    std::lock_guard<std::mutex> lock(s_AssetsMutex);
    s_Assets.clear();

    std::lock_guard<std::mutex> gpuLock(s_GPUQueueMutex);
    s_GPUUploadQueue.clear();

    CH_CORE_INFO("Assets System Shut Down");
}

void Assets::Update()
{
    std::vector<std::shared_ptr<Asset>> toUpload;
    {
        std::lock_guard<std::mutex> lock(s_GPUQueueMutex);
        if (s_GPUUploadQueue.empty())
            return;
        toUpload = std::move(s_GPUUploadQueue);
        s_GPUUploadQueue.clear();
    }

    for (auto &asset : toUpload)
    {
        if (asset->GetType() == AssetType::Texture)
        {
            auto tex = std::static_pointer_cast<TextureAsset>(asset);
            tex->UploadToGPU();
        }
        else if (asset->GetType() == AssetType::Model)
        {
            auto model = std::static_pointer_cast<ModelAsset>(asset);
            model->UploadToGPU();
        }
    }
}

void Assets::QueueForGPUUpload(std::shared_ptr<Asset> asset)
{
    std::lock_guard<std::mutex> lock(s_GPUQueueMutex);
    s_GPUUploadQueue.push_back(asset);
}

std::shared_ptr<ShaderAsset> Assets::LoadShader(const std::string &vsPath,
                                                const std::string &fsPath)
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

std::shared_ptr<ShaderAsset> Assets::LoadShader(const std::string &path)
{
    return Get<ShaderAsset>(path);
}

std::shared_ptr<EnvironmentAsset> Assets::LoadEnvironment(const std::string &path)
{
    return Get<EnvironmentAsset>(path);
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
