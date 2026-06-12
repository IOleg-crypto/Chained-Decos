#ifndef CH_ID_COMPONENT_H
#define CH_ID_COMPONENT_H

#include "engine/foundation/uuid.h"
#include "engine/reflection/reflection_rfl.h"

namespace Chained
{
struct IDComponent
{
    UUID ID;
};
CH_MARK_RFL(IDComponent);
} // namespace Chained

#endif // CH_ID_COMPONENT_H
