#include "asset_manager.h"
#include "engine/core/log.h"
#include "engine/core/thread_dispatcher.h"
#include "engine/core/types.h"
#include "engine/scene/project.h"
#include <filesystem>

namespace CHEngine
{
std::unordered_map<std::string, Model, AssetManager::StringHash, std::equal_to<>>
    AssetManager::s_Models;
std::unordered_map<std::string, Texture2D, AssetManager::StringHash, std::equal_to<>>
    AssetManager::s_Textures;
std::mutex AssetManager::s_ModelsMutex;
std::mutex AssetManager::s_TexturesMutex;

void AssetManager::Init()
{
    CH_CORE_INFO("Asset Manager Initialized");
}

void AssetManager::Shutdown()
{
    CH_CORE_INFO("Asset Manager Shutting Down: Unloading %d models", s_Models.size());
    for (auto &[name, model] : s_Models)
    {
        ::UnloadModel(model);
    }
    s_Models.clear();

    CH_CORE_INFO("Asset Manager Shutting Down: Unloading %d textures", s_Textures.size());
    for (auto &[name, texture] : s_Textures)
    {
        ::UnloadTexture(texture);
    }
    s_Textures.clear();
}

Model AssetManager::LoadModel(std::string_view path)
{
    // Check cache first (thread-safe)
    {
        std::lock_guard<std::mutex> lock(s_ModelsMutex);
        auto it = s_Models.find(path);
        if (it != s_Models.end())
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
            std::lock_guard<std::mutex> lock(s_ModelsMutex);
            s_Models[std::string(path)] = model;
            CH_CORE_INFO("Procedural Model Generated: %s", std::string(path).c_str());
            return model;
        }
    }

    std::filesystem::path fullPath = path;
    if (Project::GetActive() && !fullPath.is_absolute())
    {
        fullPath = Project::GetAssetDirectory() / path;
    }

    if (!std::filesystem::exists(fullPath))
    {
        // Don't log error for procedural paths that were somehow missed by the prefix check
        if (path.size() > 0 && path[0] != ':')
        {
            CH_CORE_ERROR("Model file does not exist: %s", fullPath.string().c_str());
        }
        return {0};
    }

    Model model = ::LoadModel(fullPath.string().c_str());
    if (model.meshCount > 0)
    {
        std::lock_guard<std::mutex> lock(s_ModelsMutex);
        s_Models[std::string(path)] = model;
        CH_CORE_INFO("Model Loaded: %s", fullPath.string().c_str());
    }
    else
    {
        CH_CORE_ERROR("Failed to load model: %s", fullPath.string().c_str());
    }

    return model;
}

Model AssetManager::GetModel(std::string_view name)
{
    auto it = s_Models.find(name);
    if (it != s_Models.end())
        return it->second;

    return Model{0}; // Return empty model if not found
}

bool AssetManager::HasModel(std::string_view name)
{
    std::lock_guard<std::mutex> lock(s_ModelsMutex);
    return s_Models.find(name) != s_Models.end();
}

void AssetManager::UnloadModel(std::string_view name)
{
    std::lock_guard<std::mutex> lock(s_ModelsMutex);
    auto it = s_Models.find(name);
    if (it != s_Models.end())
    {
        ::UnloadModel(it->second);
        s_Models.erase(it);
        CH_CORE_INFO("Model Unloaded: %s", std::string(name).c_str());
    }
}

Texture2D AssetManager::LoadTexture(std::string_view path)
{
    {
        std::lock_guard<std::mutex> lock(s_TexturesMutex);
        auto it = s_Textures.find(path);
        if (it != s_Textures.end())
            return it->second;
    }

    std::filesystem::path fullPath = path;
    if (Project::GetActive() && !fullPath.is_absolute())
    {
        fullPath = Project::GetAssetDirectory() / path;
    }

    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("Texture file does not exist: %s", fullPath.string().c_str());
        return {0};
    }

    Texture2D texture = ::LoadTexture(fullPath.string().c_str());
    if (texture.id > 0)
    {
        std::lock_guard<std::mutex> lock(s_TexturesMutex);
        s_Textures[std::string(path)] = texture;
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
    auto it = s_Textures.find(name);
    if (it != s_Textures.end())
        return it->second;

    return Texture2D{0};
}

bool AssetManager::HasTexture(std::string_view name)
{
    std::lock_guard<std::mutex> lock(s_TexturesMutex);
    return s_Textures.find(name) != s_Textures.end();
}

void AssetManager::UnloadTexture(std::string_view name)
{
    std::lock_guard<std::mutex> lock(s_TexturesMutex);
    auto it = s_Textures.find(name);
    if (it != s_Textures.end())
    {
        ::UnloadTexture(it->second);
        s_Textures.erase(it);
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
    // Check cache first (thread-safe)
    {
        std::lock_guard<std::mutex> lock(s_ModelsMutex);
        auto it = s_Models.find(path);
        if (it != s_Models.end())
        {
            ModelLoadResult result{it->second, std::string(path), true};
            std::promise<ModelLoadResult> promise;
            promise.set_value(result);
            return promise.get_future();
        }
    }

    std::string pathStr(path);
    // Enqueue background loading task
    return ThreadDispatcher::DispatchAsync(
        [pathStr]() -> ModelLoadResult
        {
            ModelLoadResult result;
            result.path = pathStr;
            result.success = false;

            // Check for procedural models
            if (pathStr.size() > 0 && pathStr[0] == ':')
            {
                Mesh mesh = {0};
                if (pathStr == ":cube:")
                    mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
                else if (pathStr == ":sphere:")
                    mesh = GenMeshSphere(0.5f, 16, 16);
                else if (pathStr == ":plane:")
                    mesh = GenMeshPlane(10.0f, 10.0f, 10, 10);

                if (mesh.vertexCount > 0)
                {
                    Model model = LoadModelFromMesh(mesh);
                    result.model = model;
                    result.success = true;

                    std::lock_guard<std::mutex> lock(s_ModelsMutex);
                    s_Models[pathStr] = model;
                    CH_CORE_INFO("Procedural Model Generated Async: %s", pathStr.c_str());
                    return result;
                }
            }

            // Load from file
            std::filesystem::path fullPath = pathStr;
            if (Project::GetActive() && !fullPath.is_absolute())
                fullPath = Project::GetAssetDirectory() / pathStr;

            if (!std::filesystem::exists(fullPath))
            {
                CH_CORE_ERROR("Model file does not exist: %s", fullPath.string().c_str());
                return result;
            }

            Model model = ::LoadModel(fullPath.string().c_str());
            if (model.meshCount > 0)
            {
                result.model = model;
                result.success = true;

                std::lock_guard<std::mutex> lock(s_ModelsMutex);
                s_Models[pathStr] = model;
                CH_CORE_INFO("Model Loaded Async: %s", fullPath.string().c_str());
            }
            else
            {
                CH_CORE_ERROR("Failed to load model async: %s", pathStr.c_str());
            }

            return result;
        });
}

void AssetManager::LoadModelsAsync(const std::vector<std::string> &paths,
                                   std::function<void(float)> progressCallback)
{
    if (paths.empty())
        return;

    std::vector<std::future<ModelLoadResult>> futures;
    futures.reserve(paths.size());

    // Enqueue all loads
    CH_CORE_INFO("Starting parallel load of %d models...", paths.size());
    for (const auto &path : paths)
    {
        futures.push_back(LoadModelAsync(path));
    }

    // Wait and report progress
    size_t completed = 0;
    for (auto &future : futures)
    {
        future.wait();
        completed++;
        if (progressCallback)
            progressCallback(static_cast<float>(completed) / paths.size());
    }

    CH_CORE_INFO("Parallel model loading complete: %d/%d models loaded", completed, paths.size());
}

bool AssetManager::IsModelReady(std::string_view path)
{
    std::lock_guard<std::mutex> lock(s_ModelsMutex);
    return s_Models.find(path) != s_Models.end();
}

} // namespace CHEngine
