#pragma once

#include <cstdint>

namespace ECS
{

using EntityID = uint32_t;
const EntityID MAX_ENTITIES = 10000;
const EntityID NULL_ENTITY = 0xFFFFFFFF;

} // namespace ECS
