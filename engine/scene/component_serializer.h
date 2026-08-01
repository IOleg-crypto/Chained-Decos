#ifndef CH_COMPONENT_SERIALIZER_H
#define CH_COMPONENT_SERIALIZER_H

#include "engine/core/service.h"
#include "engine/scene/component_registry.h"
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Chained
{
// Central helper for component serialization used by scene save/load/copy.
class ComponentSerializer
{
public:
    // Serializes all registered components owned by an entity.
    static void SerializeAll(YAML::Emitter& out, Entity entity);

    // Deserializes all registered components from YAML.
    static void DeserializeAll(Entity entity, YAML::Node node);

    // Copies all registered components from source to destination.
    static void CopyAll(Entity source, Entity destination);

    // Serializes the ID component separately.
    static void SerializeID(YAML::Emitter& out, Entity entity);
};

} // namespace Chained

#endif // CH_COMPONENT_SERIALIZER_H
