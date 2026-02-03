#include "entity.h"
#include "scene.h"
#include "components/id_component.h"
#include "components/tag_component.h"

namespace CHEngine
{
    UUID Entity::GetUUID()
    {
        return GetComponent<IDComponent>().ID;
    }

    const std::string &Entity::GetName()
    {
        return GetComponent<TagComponent>().Tag;
    }

    bool Entity::IsValid() const
    {
        return m_EntityHandle != entt::null && m_Scene != nullptr && m_Scene->GetRegistry().valid(m_EntityHandle);
    }
}
