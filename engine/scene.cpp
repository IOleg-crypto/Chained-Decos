#include "scene.h"
#include "components.h"
#include "entity.h"

namespace CH
{
Scene::Scene()
{
}

Entity Scene::CreateEntity(const std::string &name)
{
    Entity entity(m_Registry.create(), this);
    entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
    entity.AddComponent<TransformComponent>();

    CH_CORE_INFO("Entity Created: %s (%d)", name, (uint32_t)entity);
    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    m_Registry.destroy(entity);
}

void Scene::OnUpdateRuntime(float deltaTime)
{
    // TODO: Update systems
}

void Scene::OnUpdateEditor(float deltaTime)
{
    // TODO: Editor specific updates
}
} // namespace CH
