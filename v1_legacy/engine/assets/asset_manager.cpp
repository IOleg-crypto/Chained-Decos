#include "asset_manager.h"
#include "core/log.h"
#include "engine/scene/resources/model/model.h"

namespace CHEngine
{
bool AssetManager::LoadModel(const std::string &name, const std::string &path, bool preload)
{
    if (ModelLoader::IsInitialized())
        return ModelLoader::LoadSingleModel(name, path, preload);
    CD_CORE_ERROR("AssetManager::LoadModel - ModelLoader not initialized!");
    return false;
}

void AssetManager::UnloadAllModels()
{
    if (ModelLoader::IsInitialized())
        ModelLoader::UnloadAllModels();
}

std::vector<std::string> AssetManager::GetAvailableModels()
{
    return ModelLoader::IsInitialized() ? ModelLoader::GetAvailableModels()
                                        : std::vector<std::string>{};
}

std::optional<std::reference_wrapper<Model>> AssetManager::GetModel(const std::string &name)
{
    return ModelLoader::IsInitialized() ? ModelLoader::GetModelByName(name) : std::nullopt;
}

bool AssetManager::LoadFont(const std::string &name, const std::string &path)
{
    return false;
}

Font AssetManager::GetFont(const std::string &name)
{
    return GetFontDefault();
}
} // namespace CHEngine
