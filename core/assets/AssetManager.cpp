#include "AssetManager.h"
#include "core/Log.h"
#include "scene/resources/model/Model.h"
#include "scene/resources/model/interfaces/IModelLoader.h"


namespace CHEngine
{

bool AssetManager::LoadModel(const std::string &name, const std::string &path, bool preload)
{
    if (ModelLoader::IsInitialized())
    {
        return ModelLoader::LoadSingleModel(name, path, preload);
    }
    CD_CORE_ERROR("AssetManager::LoadModel - ModelLoader not initialized!");
    return false;
}

void AssetManager::UnloadAllModels()
{
    if (ModelLoader::IsInitialized())
    {
        ModelLoader::UnloadAllModels();
    }
}

std::vector<std::string> AssetManager::GetAvailableModels()
{
    if (ModelLoader::IsInitialized())
    {
        return ModelLoader::GetAvailableModels();
    }
    return {};
}

std::optional<std::reference_wrapper<Model>> AssetManager::GetModel(const std::string &name)
{
    if (ModelLoader::IsInitialized())
    {
        return ModelLoader::GetModelByName(name);
    }
    return std::nullopt;
}

bool AssetManager::LoadFont(const std::string &name, const std::string &path)
{
    // TODO: Implement FontService wrapper if needed
    (void)name;
    (void)path;
    return false;
}

Font AssetManager::GetFont(const std::string &name)
{
    // TODO: Implement FontService wrapper if needed
    (void)name;
    return GetFontDefault();
}

} // namespace CHEngine
