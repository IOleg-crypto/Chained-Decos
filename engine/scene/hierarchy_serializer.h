#ifndef CH_HIERARCHY_SERIALIZER_H
#define CH_HIERARCHY_SERIALIZER_H

#include "engine/scene/scene.h"
#include <yaml-cpp/yaml.h>

namespace CHEngine
{
struct HierarchyTask
{
    Entity entity;
    uint64_t parent;
    std::vector<uint64_t> children;
};

class HierarchySerializer
{
public:
    static void Serialize(YAML::Emitter& out, Entity entity);
    static void DeserializeTask(Entity entity, YAML::Node node, HierarchyTask& outTask);
};

} // namespace CHEngine

#endif // CH_HIERARCHY_SERIALIZER_H
