#include "Entity.h"
#include "Scene.h"
#include "core/Log.h"

namespace CHEngine
{

Entity::Entity(entt::entity handle, Scene *scene) : m_EntityHandle(handle), m_Scene(scene)
{
}

} // namespace CHEngine
