#include "engine/audio/sound_asset.h"
#include "engine/core/base.h"
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

    // Cache Access
    static Ref<ModelAsset> LoadModel(const std::string &path);
    static Ref<TextureAsset> LoadTexture(const std::string &path);
    static Ref<SoundAsset> LoadSound(const std::string &path);
    static Ref<ShaderAsset> LoadShader(const std::string &vsPath, const std::string &fsPath);

    // Async
    static std::future<Ref<ModelAsset>> LoadModelAsync(const std::string &path);

    // Helpers
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

    static std::unordered_map<std::string, Ref<ModelAsset>, StringHash, std::equal_to<>> s_Models;
    static std::unordered_map<std::string, Ref<TextureAsset>, StringHash, std::equal_to<>>
        s_Textures;
    static std::unordered_map<std::string, Ref<SoundAsset>, StringHash, std::equal_to<>> s_Sounds;
    static std::unordered_map<std::string, Ref<ShaderAsset>, StringHash, std::equal_to<>> s_Shaders;

    static std::mutex s_ModelsMutex;
    static std::mutex s_TexturesMutex;
    static std::mutex s_SoundsMutex;
    static std::mutex s_ShadersMutex;
};

// Aliasing for compatibility during transition if needed
using AssetManager = Assets;

} // namespace CHEngine
