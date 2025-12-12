#include "Component.h"
#include "GameObject.h"

Component::Component(GameObject *owner) : m_owner(owner), m_enabled(true)
{
}

void Component::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool Component::IsEnabled() const
{
    return m_enabled;
}
