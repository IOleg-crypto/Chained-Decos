#include "entity.h"
#include "core/log.h"
#include "scene.h"


namespace CHEngine
{

Entity::Entity(entt::entity handle, Scene *scene) : m_EntityHandle(handle), m_Scene(scene)
{
}

} // namespace CHEngine
