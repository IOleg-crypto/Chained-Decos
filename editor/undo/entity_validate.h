#ifndef CH_ENTITY_VALIDATE_H
#define CH_ENTITY_VALIDATE_H

#include "engine/scene/entity.h"

namespace Chained
{

inline bool ValidateEntity(Entity& entity)
{
    if (!entity)
    {
        return false;
    }
    auto& registry = entity.GetRegistry();
    return registry.valid(static_cast<entt::entity>(entity));
}

} // namespace Chained

#endif // CH_ENTITY_VALIDATE_H
