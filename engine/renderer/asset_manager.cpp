#include "asset_manager.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/log.h"
#include "engine/core/thread_dispatcher.h"
#include "engine/scene/project.h"
#include <fstream>

namespace CHEngine
{

std::unordered_map<std::string, Ref<ModelAsset>, Assets::StringHash, std::equal_to<>>
    Assets::s_Models;
std::unordered_map<std::string, Ref<TextureAsset>, Assets::StringHash, std::equal_to<>>
    Assets::s_Textures;
std::unordered_map<std::string, Ref<SoundAsset>, Assets::StringHash, std::equal_to<>>
    Assets::s_Sounds;
std::unordered_map<std::string, Ref<ShaderAsset>, Assets::StringHash, std::equal_to<>>
    Assets::s_Shaders;
std::mutex Assets::s_ModelsMutex;
std::mutex Assets::s_TexturesMutex;
std::mutex Assets::s_SoundsMutex;
std::mutex Assets::s_ShadersMutex;

void Assets::Init()
{
    CH_CORE_INFO("Assets System Initialized");
}

void Assets::Shutdown()
{
    {
        std::lock_guard<std::mutex> lock(s_ModelsMutex);
        s_Models.clear();
    }
    {
        std::lock_guard<std::mutex> lock(s_TexturesMutex);
        s_Textures.clear();
    }
    {
        std::lock_guard<std::mutex> lock(s_SoundsMutex);
        s_Sounds.clear();
    }
    {
        std::lock_guard<std::mutex> lock(s_ShadersMutex);
        s_Shaders.clear();
    }
    CH_CORE_INFO("Assets System Shut Down");
}

Ref<ModelAsset> Assets::LoadModel(const std::string &path)
{
    {
        std::lock_guard<std::mutex> lock(s_ModelsMutex);
        auto it = s_Models.find(path);
        if (it != s_Models.end())
            return it->second;
    }

    Ref<ModelAsset> asset;
    if (path.size() > 0 && path[0] == ':')
        asset = ModelAsset::CreateProcedural(path);
    else
        asset = ModelAsset::Load(path);

    if (asset)
    {
        std::lock_guard<std::mutex> lock(s_ModelsMutex);
        s_Models[path] = asset;
    }
    return asset;
}

Ref<TextureAsset> Assets::LoadTexture(const std::string &path)
{
    {
        std::lock_guard<std::mutex> lock(s_TexturesMutex);
        auto it = s_Textures.find(path);
        if (it != s_Textures.end())
            return it->second;
    }

    auto asset = TextureAsset::Load(path);
    if (asset)
    {
        std::lock_guard<std::mutex> lock(s_TexturesMutex);
        s_Textures[path] = asset;
    }
    return asset;
}

Ref<SoundAsset> Assets::LoadSound(const std::string &path)
{
    std::lock_guard<std::mutex> lock(s_SoundsMutex);
    if (s_Sounds.find(path) != s_Sounds.end())
        return s_Sounds[path];

    auto asset = SoundAsset::Load(path);
    if (asset)
        s_Sounds[path] = asset;
    return asset;
}

Ref<ShaderAsset> Assets::LoadShader(const std::string &vsPath, const std::string &fsPath)
{
    std::string key = vsPath + "|" + fsPath;
    {
        std::lock_guard<std::mutex> lock(s_ShadersMutex);
        if (s_Shaders.count(key))
            return s_Shaders[key];
    }

    auto asset = ShaderAsset::Load(vsPath, fsPath);
    if (asset)
    {
        std::lock_guard<std::mutex> lock(s_ShadersMutex);
        s_Shaders[key] = asset;
    }
    return asset;
}

std::future<Ref<ModelAsset>> Assets::LoadModelAsync(const std::string &path)
{
    auto promise = std::make_shared<std::promise<Ref<ModelAsset>>>();

    // Check cache first (main thread)
    {
        std::lock_guard<std::mutex> lock(s_ModelsMutex);
        if (s_Models.count(path))
        {
            promise->set_value(s_Models[path]);
            return promise->get_future();
        }
    }

    ThreadDispatcher::DispatchAsync(
        [path, promise]()
        {
            // Do some "warmup" work in background (disk IO)
            auto fullPath = ResolvePath(path);
            if (std::filesystem::exists(fullPath))
            {
                // We could read the file here to warm up OS cache,
                // but Raylib's LoadModel will still do its own IO.
                // At least checking existence is offloaded.
                std::ifstream f(fullPath, std::ios::binary | std::ios::ate);
                if (f.is_open())
                {
                    // Just reading the size and a bit of data is a good "touch"
                    // for the OS disk cache.
                    auto size = f.tellg();
                }
            }

            // Dispatch to main thread for the actual Raylib/OpenGL upload
            ThreadDispatcher::DispatchMain([path, promise]()
                                           { promise->set_value(Assets::LoadModel(path)); });
        });

    return promise->get_future();
}

std::filesystem::path Assets::ResolvePath(const std::string &path)
{
    if (path.empty())
        return "";

    if (path.starts_with("engine:"))
        return (std::filesystem::path(PROJECT_ROOT_DIR) / "engine/resources" / path.substr(7))
            .make_preferred();

    std::filesystem::path fullPath = path;
    if (Project::GetActive() && !fullPath.is_absolute())
        return (Project::GetAssetDirectory() / path).make_preferred();

    return fullPath.make_preferred();
}

} // namespace CHEngine
