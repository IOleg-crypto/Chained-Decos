#ifndef CH_ID_COMPONENT_H
#define CH_ID_COMPONENT_H

#include "engine/core/uuid.h"
#include "engine/core/reflection_rfl.h"

namespace CHEngine
{
struct IDComponent
{
    UUID ID;
};
CH_MARK_RFL(IDComponent);
} // namespace CHEngine

#endif // CH_ID_COMPONENT_H
