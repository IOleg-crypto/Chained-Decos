#include "Scene.h"

namespace CHEngine
{

Scene::Scene() : m_name("Untitled Scene")
{
}

Scene::~Scene()
{
}

nlohmann::json Scene::ToJson() const
{
    nlohmann::json j;
    j["name"] = m_name;
    return j;
}

void Scene::FromJson(const nlohmann::json &j)
{
    m_name = j.value("name", "Untitled Scene");
}

} // namespace CHEngine
