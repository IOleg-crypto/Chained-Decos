#ifndef CH_ID_COMPONENT_H
#define CH_ID_COMPONENT_H

#include "engine/common/uuid.h"
#include "engine/reflection/reflection_rfl.h"

namespace Chained
{
struct IDComponent
{
    UUID ID;

    static const char* GetStaticName() { return "IDComponent"; }

    
    struct UI
    {
        UIMeta ID = {.Tooltip = "Unique entity identifier (UUID)"};
    };
};
CH_MARK_RFL(IDComponent);
} // namespace Chained

#endif // CH_ID_COMPONENT_H
