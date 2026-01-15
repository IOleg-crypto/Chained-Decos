#include "asset_manager.h"
#include "engine/core/base.h"
#include "engine/core/log.h"
#include "engine/core/thread_dispatcher.h"
#include "engine/scene/project.h"
#include <filesystem>

namespace CHEngine
{
std::unordered_map<std::string, Model, AssetManager::StringHash, std::equal_to<>>
    AssetManager::s_LoadedModels;
std::unordered_map<std::string, Texture2D, AssetManager::StringHash, std::equal_to<>>
    AssetManager::s_LoadedTextures;
std::unordered_map<std::string, std::pair<ModelAnimation *, int>, AssetManager::StringHash,
                   std::equal_to<>>
    AssetManager::s_LoadedAnimations;
std::mutex AssetManager::s_ModelsCacheMutex;
std::mutex AssetManager::s_TexturesCacheMutex;
std::mutex AssetManager::s_AnimationsCacheMutex;

void AssetManager::Init()
{
    CH_CORE_INFO("Asset Manager Initialized");
}

void AssetManager::Shutdown()
{
    CH_CORE_INFO("Asset Manager Shutting Down: Unloading %d models", s_LoadedModels.size());
    for (auto &[name, model] : s_LoadedModels)
    {
        ::UnloadModel(model);
    }
    s_LoadedModels.clear();

    CH_CORE_INFO("Asset Manager Shutting Down: Unloading %d textures", s_LoadedTextures.size());
    for (auto &[name, texture] : s_LoadedTextures)
    {
        ::UnloadTexture(texture);
    }
    s_LoadedTextures.clear();

    CH_CORE_INFO("Asset Manager Shutting Down: Unloading %d animation sets",
                 s_LoadedAnimations.size());
    for (auto &[name, animPair] : s_LoadedAnimations)
    {
        ::UnloadModelAnimations(animPair.first, animPair.second);
    }
    s_LoadedAnimations.clear();
}

Model AssetManager::LoadModel(std::string_view path)
{
    if (path.empty())
        return {0};

    if (!ThreadDispatcher::IsMainThread())
    {
        CH_CORE_WARN("AssetManager::LoadModel called from non-main thread! This may cause "
                     "Raylib/OpenGL crashes. Path: {0}",
                     path);
    }

    // Check cache first (thread-safe)
    {
        std::lock_guard<std::mutex> lock(s_ModelsCacheMutex);
        auto it = s_LoadedModels.find(path);
        if (it != s_LoadedModels.end())
            return it->second;
    }

    // Check for procedural first before prepending project path
    if (path.size() > 0 && path[0] == ':')
    {
        Mesh mesh = {0};
        if (path == ":cube:")
            mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
        else if (path == ":sphere:")
            mesh = GenMeshSphere(0.5f, 16, 16);
        else if (path == ":plane:")
            mesh = GenMeshPlane(10.0f, 10.0f, 10, 10);
        else if (path == ":cylinder:")
            mesh = GenMeshCylinder(0.5f, 1.0f, 16);
        else if (path == ":cone:")
            mesh = GenMeshCone(0.5f, 1.0f, 16);
        else if (path == ":torus:")
            mesh = GenMeshTorus(0.3f, 0.7f, 16, 16);
        else if (path == ":knot:")
            mesh = GenMeshKnot(0.3f, 0.7f, 16, 16);

        if (mesh.vertexCount > 0)
        {
            Model model = LoadModelFromMesh(mesh);
            std::lock_guard<std::mutex> lock(s_ModelsCacheMutex);
            s_LoadedModels[std::string(path)] = model;
            CH_CORE_INFO("Procedural Model Generated: %s", std::string(path).c_str());
            return model;
        }
    }

    std::filesystem::path fullPath = ResolvePath(path);

    if (!std::filesystem::exists(fullPath))
    {
        // Don't log error for procedural paths that were somehow missed by the prefix check
        if (path.size() > 0 && path[0] != ':')
        {
            CH_CORE_ERROR("Model file does not exist: %s (Original: %s)", fullPath.string().c_str(),
                          std::string(path).c_str());
        }
        return {0};
    }

    Model model = ::LoadModel(fullPath.string().c_str());
    if (model.meshCount > 0)
    {
        std::lock_guard<std::mutex> lock(s_ModelsCacheMutex);
        s_LoadedModels[std::string(path)] = model;
        CH_CORE_INFO("Model Loaded: %s", fullPath.string().c_str());

        // Load animations if any
        int animCount = 0;
        ModelAnimation *animations = ::LoadModelAnimations(fullPath.string().c_str(), &animCount);
        if (animations != nullptr)
        {
            std::lock_guard<std::mutex> animLock(s_AnimationsCacheMutex);
            s_LoadedAnimations[std::string(path)] = {animations, animCount};
            CH_CORE_INFO("Loaded %d animations for: %s", animCount, fullPath.string().c_str());
        }
    }
    else
    {
        CH_CORE_ERROR("Failed to load model: %s", fullPath.string().c_str());
    }

    return model;
}

Model AssetManager::GetModel(std::string_view name)
{
    auto it = s_LoadedModels.find(name);
    if (it != s_LoadedModels.end())
        return it->second;

    return Model{0}; // Return empty model if not found
}

bool AssetManager::HasModel(std::string_view name)
{
    std::lock_guard<std::mutex> lock(s_ModelsCacheMutex);
    return s_LoadedModels.find(name) != s_LoadedModels.end();
}

void AssetManager::UnloadModel(std::string_view name)
{
    std::lock_guard<std::mutex> lock(s_ModelsCacheMutex);
    auto it = s_LoadedModels.find(name);
    if (it != s_LoadedModels.end())
    {
        ::UnloadModel(it->second);
        s_LoadedModels.erase(it);

        // Unload animations too
        std::lock_guard<std::mutex> animLock(s_AnimationsCacheMutex);
        auto animIt = s_LoadedAnimations.find(name);
        if (animIt != s_LoadedAnimations.end())
        {
            ::UnloadModelAnimations(animIt->second.first, animIt->second.second);
            s_LoadedAnimations.erase(animIt);
        }

        CH_CORE_INFO("Model and Animations Unloaded: %s", std::string(name).c_str());
    }
}

ModelAnimation *AssetManager::LoadAnimation(std::string_view path, int *count)
{
    if (path.empty())
    {
        if (count)
            *count = 0;
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(s_AnimationsCacheMutex);
        auto it = s_LoadedAnimations.find(path);
        if (it != s_LoadedAnimations.end())
        {
            *count = it->second.second;
            return it->second.first;
        }
    }

    // If not found in cache, try to load the model which will also load animations
    // This is a bit of a workaround, as animations are tied to models in Raylib
    // and loaded as a side effect of LoadModel.
    // A more robust solution might involve a separate animation loading mechanism.
    Model model = LoadModel(path); // This will attempt to load the model and its animations
    if (model.meshCount > 0)
    {
        // Check cache again after LoadModel might have populated it
        std::lock_guard<std::mutex> lock(s_AnimationsCacheMutex);
        auto it = s_LoadedAnimations.find(path);
        if (it != s_LoadedAnimations.end())
        {
            *count = it->second.second;
            return it->second.first;
        }
    }

    if (count)
        *count = 0;
    return nullptr;
}

ModelAnimation *AssetManager::GetAnimations(std::string_view path, int *count)
{
    std::lock_guard<std::mutex> lock(s_AnimationsCacheMutex);
    auto it = s_LoadedAnimations.find(path);
    if (it != s_LoadedAnimations.end())
    {
        *count = it->second.second;
        return it->second.first;
    }

    *count = 0;
    return nullptr;
}

Texture2D AssetManager::LoadTexture(std::string_view path)
{
    if (path.empty())
        return {0};

    {
        std::lock_guard<std::mutex> lock(s_TexturesCacheMutex);
        auto it = s_LoadedTextures.find(path);
        if (it != s_LoadedTextures.end())
            return it->second;
    }

    std::filesystem::path fullPath = ResolvePath(path);

    if (!std::filesystem::exists(fullPath))
    {
        // Try fallback: search in Textures/ directory using just the filename
        std::filesystem::path filename = std::filesystem::path(path).filename();
        std::filesystem::path fallbackPath = ResolvePath("Textures/" + filename.string());

        if (std::filesystem::exists(fallbackPath))
        {
            CH_CORE_WARN("Texture not found at '%s', using fallback: %s", fullPath.string().c_str(),
                         fallbackPath.string().c_str());
            fullPath = fallbackPath;
        }
        else
        {
            CH_CORE_ERROR("Texture file does not exist: %s (Original: %s)",
                          fullPath.string().c_str(), std::string(path).c_str());
            return {0};
        }
    }

    Texture2D texture = ::LoadTexture(fullPath.string().c_str());
    if (texture.id > 0)
    {
        std::lock_guard<std::mutex> lock(s_TexturesCacheMutex);
        s_LoadedTextures[std::string(path)] = texture;
        CH_CORE_INFO("Texture Loaded: %s", fullPath.string().c_str());
    }
    else
    {
        CH_CORE_ERROR("Failed to load texture: %s", fullPath.string().c_str());
    }

    return texture;
}

Texture2D AssetManager::GetTexture(std::string_view name)
{
    auto it = s_LoadedTextures.find(name);
    if (it != s_LoadedTextures.end())
        return it->second;

    return Texture2D{0};
}

bool AssetManager::HasTexture(std::string_view name)
{
    std::lock_guard<std::mutex> lock(s_TexturesCacheMutex);
    return s_LoadedTextures.find(name) != s_LoadedTextures.end();
}

void AssetManager::UnloadTexture(std::string_view name)
{
    std::lock_guard<std::mutex> lock(s_TexturesCacheMutex);
    auto it = s_LoadedTextures.find(name);
    if (it != s_LoadedTextures.end())
    {
        ::UnloadTexture(it->second);
        s_LoadedTextures.erase(it);
        CH_CORE_INFO("Texture Unloaded: %s", std::string(name).c_str());
    }
}

Texture2D AssetManager::LoadCubemap(std::string_view path)
{
    // Use LoadTexture for now, but in Renderer we will handle the 2D to Cubemap conversion
    // logic if we detect it's a 2D panorama.
    return LoadTexture(path);
}
BoundingBox AssetManager::GetModelBoundingBox(std::string_view path)
{
    Model model = LoadModel(path);
    if (model.meshCount > 0)
    {
        return ::GetModelBoundingBox(model);
    }
    return BoundingBox{{0, 0, 0}, {0, 0, 0}};
}

std::future<ModelLoadResult> AssetManager::LoadModelAsync(std::string_view path)
{
    if (path.empty())
    {
        std::promise<ModelLoadResult> promise;
        promise.set_value({{0}, "", false});
        return promise.get_future();
    }

    // Fast cache check
    {
        std::lock_guard<std::mutex> lock(s_ModelsCacheMutex);
        auto it = s_LoadedModels.find(path);
        if (it != s_LoadedModels.end())
        {
            ModelLoadResult result{it->second, std::string(path), true};
            std::promise<ModelLoadResult> promise;
            promise.set_value(result);
            return promise.get_future();
        }
    }

    std::string pathStr(path);
    auto promise = std::make_shared<std::promise<ModelLoadResult>>();
    std::future<ModelLoadResult> future = promise->get_future();

    // Async preparation (IO and path checks)
    ThreadDispatcher::DispatchAsync(
        [pathStr, promise]()
        {
            // Preparation in background
            std::filesystem::path fullPath = pathStr;
            if (Project::GetActive() && !fullPath.is_absolute() && !pathStr.empty() &&
                pathStr[0] != ':')
            {
                fullPath = Project::GetAssetDirectory() / pathStr;
            }

            if (!pathStr.empty() && pathStr[0] != ':' && !std::filesystem::exists(fullPath))
            {
                CH_CORE_ERROR("Model file does not exist: %s", fullPath.string().c_str());
                promise->set_value({{0}, pathStr, false});
                return;
            }

            // Dispatch GPU upload (LoadModel) to Main Thread
            ThreadDispatcher::DispatchMain(
                [pathStr, promise]()
                {
                    // Check cache again on main thread
                    {
                        std::lock_guard<std::mutex> lock(s_ModelsCacheMutex);
                        if (s_LoadedModels.count(pathStr))
                        {
                            promise->set_value({s_LoadedModels[pathStr], pathStr, true});
                            return;
                        }
                    }

                    // Actual Raylib upload
                    Model model = AssetManager::LoadModel(pathStr);
                    bool success = model.meshCount > 0;

                    promise->set_value({model, pathStr, success});
                });
        });

    return future;
}

void AssetManager::LoadModelsAsync(const std::vector<std::string> &paths,
                                   std::function<void(float)> progressCallback)
{
    if (paths.empty())
        return;

    std::vector<std::future<ModelLoadResult>> futures;
    futures.reserve(paths.size());

    // Enqueue all loads
    CH_CORE_INFO("Starting parallel load of {0} models...", paths.size());
    for (const auto &path : paths)
    {
        futures.push_back(LoadModelAsync(path));
    }

    // Wait and report progress
    size_t completed = 0;
    bool isMainThread = ThreadDispatcher::IsMainThread();

    for (auto &future : futures)
    {
        if (isMainThread)
        {
            // If on main thread, we MUST pump the main thread queue while waiting to avoid deadlock
            while (future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            {
                ThreadDispatcher::ExecuteMainThreadQueue();
                std::this_thread::yield();
            }
        }
        else
        {
            future.wait();
        }

        completed++;
        if (progressCallback)
            progressCallback(static_cast<float>(completed) / paths.size());
    }

    CH_CORE_INFO("Parallel model loading complete: {0}/{1} models loaded", completed, paths.size());
}

bool AssetManager::IsModelReady(std::string_view path)
{
    std::lock_guard<std::mutex> lock(s_ModelsCacheMutex);
    return s_LoadedModels.find(path) != s_LoadedModels.end();
}

Shader AssetManager::LoadShader(std::string_view vsPath, std::string_view fsPath)
{
    std::filesystem::path vsFullPath = ResolvePath(vsPath);
    std::filesystem::path fsFullPath = ResolvePath(fsPath);

    return ::LoadShader(vsFullPath.string().c_str(), fsFullPath.string().c_str());
}

std::filesystem::path AssetManager::ResolvePath(std::string_view path)
{
    if (path.starts_with("engine:"))
    {
        return (std::filesystem::path(PROJECT_ROOT_DIR) / "engine/resources" / path.substr(7))
            .make_preferred();
    }

    std::filesystem::path fullPath = path;
    if (Project::GetActive() && !fullPath.is_absolute())
    {
        return (Project::GetAssetDirectory() / path).make_preferred();
    }

    return fullPath.make_preferred();
}

} // namespace CHEngine
