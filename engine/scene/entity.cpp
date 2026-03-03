#include "engine/scene/entity.h"

namespace CHEngine
{
bool Entity::IsValid() const
{
    return m_EntityHandle != entt::null && m_Registry != nullptr && m_Registry->valid(m_EntityHandle);
}

} // namespace CHEngine
