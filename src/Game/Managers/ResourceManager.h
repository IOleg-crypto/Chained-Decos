#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <vector>
#include <optional>
#include <Engine/Model/Model.h>

class ModelLoader;

class ResourceManager
{
private:
    ModelLoader* m_models;

public:
    explicit ResourceManager(ModelLoader* models);
    ~ResourceManager() = default;

    std::optional<ModelLoader::LoadResult> LoadGameModels();
    std::optional<ModelLoader::LoadResult> LoadGameModelsSelective(const std::vector<std::string> &modelNames);
    std::optional<ModelLoader::LoadResult> LoadGameModelsSelectiveSafe(const std::vector<std::string> &modelNames);
    
    std::string GetModelNameForObjectType(int objectType, const std::string &modelName = "");
    std::vector<std::string> GetModelsRequiredForMap(const std::string &mapIdentifier);
};

#endif // RESOURCE_MANAGER_H

