#include "AssetManager.h"
#include "core/Engine.h"
#include "scene/resources/model/interfaces/IModelLoader.h"

namespace CHEngine
{

bool AssetManager::LoadModel(const std::string &name, const std::string &path, bool preload)
{
    return Engine::Instance().GetModelLoader().LoadSingleModel(name, path, preload);
}

void AssetManager::UnloadAllModels()
{
    Engine::Instance().GetModelLoader().UnloadAllModels();
}

std::vector<std::string> AssetManager::GetAvailableModels()
{
    return Engine::Instance().GetModelLoader().GetAvailableModels();
}

std::optional<std::reference_wrapper<Model>> AssetManager::GetModel(const std::string &name)
{
    return Engine::Instance().GetModelLoader().GetModelByName(name);
}

bool AssetManager::LoadFont(const std::string &name, const std::string &path)
{
    return Engine::Instance().GetFontService().LoadFont(name, path);
}

Font AssetManager::GetFont(const std::string &name)
{
    return Engine::Instance().GetFontService().GetFont(name);
}

} // namespace CHEngine
