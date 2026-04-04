#ifndef CH_ID_COMPONENT_H
#define CH_ID_COMPONENT_H

#include "engine/core/uuid.h"

namespace CHEngine
{
struct IDComponent
{
    UUID ID;

    IDComponent() = default;
    IDComponent(const IDComponent&) = default;
    IDComponent(const UUID& id)
        : ID(id)
    {
    }
};
} // namespace CHEngine

#endif // CH_ID_COMPONENT_H
