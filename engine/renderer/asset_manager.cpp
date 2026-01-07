#include "asset_manager.h"
#include "engine/core/log.h"
#include "engine/core/types.h"
#include "engine/scene/project.h"
#include <filesystem>

namespace CH
{
std::unordered_map<std::string, Model> AssetManager::s_Models;
std::unordered_map<std::string, Texture2D> AssetManager::s_Textures;

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

Model AssetManager::LoadModel(const std::string &path)
{
    if (s_Models.find(path) != s_Models.end())
        return s_Models[path];

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
            s_Models[path] = model;
            CH_CORE_INFO("Procedural Model Generated: %s", path.c_str());
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
        CH_CORE_ERROR("Model file does not exist: %s", fullPath.string().c_str());
        return {0};
    }

    Model model = ::LoadModel(fullPath.string().c_str());
    if (model.meshCount > 0)
    {
        s_Models[path] = model;
        CH_CORE_INFO("Model Loaded: %s", path);
    }
    else
    {
        CH_CORE_ERROR("Failed to load model: %s", path);
    }

    return model;
}

Model AssetManager::GetModel(const std::string &name)
{
    if (s_Models.find(name) != s_Models.end())
        return s_Models[name];

    return Model{0}; // Return empty model if not found
}

bool AssetManager::HasModel(const std::string &name)
{
    return s_Models.find(name) != s_Models.end();
}

void AssetManager::UnloadModel(const std::string &name)
{
    if (s_Models.find(name) != s_Models.end())
    {
        ::UnloadModel(s_Models[name]);
        s_Models.erase(name);
        CH_CORE_INFO("Model Unloaded: %s", name);
    }
}

Texture2D AssetManager::LoadTexture(const std::string &path)
{
    if (s_Textures.find(path) != s_Textures.end())
        return s_Textures[path];

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
        s_Textures[path] = texture;
        CH_CORE_INFO("Texture Loaded: %s", path.c_str());
    }
    else
    {
        CH_CORE_ERROR("Failed to load texture: %s", path.c_str());
    }

    return texture;
}

Texture2D AssetManager::GetTexture(const std::string &name)
{
    if (s_Textures.find(name) != s_Textures.end())
        return s_Textures[name];

    return Texture2D{0};
}

bool AssetManager::HasTexture(const std::string &name)
{
    return s_Textures.find(name) != s_Textures.end();
}

void AssetManager::UnloadTexture(const std::string &name)
{
    if (s_Textures.find(name) != s_Textures.end())
    {
        ::UnloadTexture(s_Textures[name]);
        s_Textures.erase(name);
        CH_CORE_INFO("Texture Unloaded: %s", name);
    }
}

Texture2D AssetManager::LoadCubemap(const std::string &path)
{
    // Use LoadTexture for now, but in Renderer we will handle the 2D to Cubemap conversion
    // logic if we detect it's a 2D panorama.
    return LoadTexture(path);
}
BoundingBox AssetManager::GetModelBoundingBox(const std::string &path)
{
    Model model = LoadModel(path);
    if (model.meshCount > 0)
    {
        return ::GetModelBoundingBox(model);
    }
    return BoundingBox{{0, 0, 0}, {0, 0, 0}};
}

} // namespace CH
