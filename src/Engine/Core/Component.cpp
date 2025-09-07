//
// Created by olegg on 05.09.2025.
//

#include "Component.h"


void Component::SetOwner(Entity *owner) { m_owner = owner; }

Entity * Component::GetOwner() const { return m_owner; }

void Component::SetEnabled(bool enabled) { m_enabled = enabled; }