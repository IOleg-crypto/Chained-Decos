#ifndef CH_ASSET_MANAGER_H
#define CH_ASSET_MANAGER_H

#include "asset.h"
#include "engine/audio/sound_asset.h"
#include "environment.h"
#include "model_asset.h"
#include "shader_asset.h"
#include "texture_asset.h"
#include <filesystem>
#include <future>
#include <mutex>
#include <string>
#include <unordered_map>

namespace CHEngine
{

class Assets
{
public:
    static void Init();
    static void Shutdown();

    // Generic Cache Access
    template <typename T> static Ref<T> Get(const std::string &path)
    {
        static_assert(std::is_base_of<Asset, T>::value, "T must inherit from Asset");

        std::lock_guard<std::mutex> lock(s_AssetsMutex);

        if (s_Assets.find(path) != s_Assets.end())
        {
            return std::static_pointer_cast<T>(s_Assets[path]);
        }

        Ref<T> asset = T::Load(path);
        if (asset)
        {
            s_Assets[path] = asset;
        }
        return asset;
    }

    // Helpers
    static Ref<ShaderAsset> LoadShader(const std::string &vsPath, const std::string &fsPath);
    static Ref<ShaderAsset> LoadShader(const std::string &path);
    static Ref<EnvironmentAsset> LoadEnvironment(const std::string &path);
    static std::future<Ref<ModelAsset>> LoadModelAsync(const std::string &path);
    static std::filesystem::path ResolvePath(const std::string &path);

private:
    struct StringHash
    {
        using is_transparent = void;
        size_t operator()(std::string_view txt) const
        {
            return std::hash<std::string_view>{}(txt);
        }
    };

    static std::unordered_map<std::string, Ref<Asset>, StringHash, std::equal_to<>> s_Assets;
    static std::mutex s_AssetsMutex;
};

using AssetManager = Assets;

} // namespace CHEngine

#endif // CH_ASSET_MANAGER_H
