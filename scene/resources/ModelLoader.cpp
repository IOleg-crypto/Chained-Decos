#include "ModelLoader.h"

namespace Scene
{

ModelLoader::~ModelLoader()
{
    UnloadAll();
}

bool ModelLoader::Load(const std::string &name, const std::string &path)
{
    if (Exists(name))
    {
        return true;
    }

    Model model = LoadModel(path.c_str());
    if (model.meshCount > 0)
    {
        m_models[name] = model;
        return true;
    }

    return false;
}

Model *ModelLoader::Get(const std::string &name)
{
    auto it = m_models.find(name);
    if (it != m_models.end())
    {
        return &it->second;
    }
    return nullptr;
}

void ModelLoader::Unload(const std::string &name)
{
    auto it = m_models.find(name);
    if (it != m_models.end())
    {
        UnloadModel(it->second);
        m_models.erase(it);
    }
}

void ModelLoader::UnloadAll()
{
    for (auto &[name, model] : m_models)
    {
        UnloadModel(model);
    }
    m_models.clear();
}

bool ModelLoader::Exists(const std::string &name) const
{
    return m_models.find(name) != m_models.end();
}

} // namespace Scene




