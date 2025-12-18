#include "ModelManager.h"
#include "../../../scene/resources/model/Core/Model.h"

ModelManager::ModelManager(std::shared_ptr<ModelLoader> modelLoader)
    : m_modelLoader(std::move(modelLoader))
{
}

bool ModelManager::LoadModel(const std::string &name, const std::string &path)
{
    if (!m_modelLoader)
        return false;
    return m_modelLoader->LoadSingleModel(name, path, true);
}

bool ModelManager::HasModel(const std::string &name) const
{
    if (!m_modelLoader)
        return false;
    return m_modelLoader->GetModelByName(name).has_value();
}

std::vector<std::string> ModelManager::GetAvailableModels() const
{
    if (!m_modelLoader)
        return {};
    return m_modelLoader->GetAvailableModels();
}

ModelLoader &ModelManager::GetModelLoader()
{
    return *m_modelLoader;
}

const ModelLoader &ModelManager::GetModelLoader() const
{
    return *m_modelLoader;
}