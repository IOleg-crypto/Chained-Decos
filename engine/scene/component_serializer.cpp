#include "component_serializer.h"
#include "components/control_component.h"
#include "components/id_component.h"
#include "components/ui_action_component.h"
#include "engine/scene/component_registry.h"
#include "engine/scene/hierarchy_serializer.h"
#include "managed_script_serializer.h"

namespace Chained
{

// --- Special Serialization Helpers ---

void ComponentSerializer::SerializeID(YAML::Emitter& out, Entity entity)
{
    if (entity.HasComponent<IDComponent>())
    {
        out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entity.GetComponent<IDComponent>().ID;
    }
    else
    {
        out << YAML::Key << "Entity" << YAML::Value << 0;
    }
}

// --- Registry Initialization ---

void ComponentSerializer::SerializeAll(YAML::Emitter& out, Entity entity)
{
    for (auto& [id, metadata] : ComponentRegistry::GetRegistry())
    {
        if (metadata.Serialize)
        {
            metadata.Serialize(out, entity);
        }
        else if (metadata.IsReflective && metadata.ReflectInternal && metadata.Has && metadata.Has(entity))
        {
            // Wrap in a YAML key matching SerializationKey so DeserializeAll can find it
            out << YAML::Key << metadata.SerializationKey << YAML::Value << YAML::BeginMap;
            Serialization::PropertyArchive archive(out);
            metadata.ReflectInternal(entity, &archive, (int)ReflectionMode::Serialize);
            out << YAML::EndMap;
        }
    }

    HierarchySerializer::Serialize(out, entity);
}

void ComponentSerializer::DeserializeAll(Entity entity, YAML::Node node)
{
    for (auto& [id, metadata] : ComponentRegistry::GetRegistry())
    {
        if (metadata.Deserialize)
        {
            metadata.Deserialize(entity, node);
        }
        else if (metadata.IsReflective && metadata.ReflectInternal)
        {
            // Only try if the node exists for this component
            if (node[metadata.SerializationKey])
            {
                Serialization::PropertyArchive archive(node[metadata.SerializationKey]);
                metadata.ReflectInternal(entity, &archive, (int)ReflectionMode::Deserialize);
            }
        }
    }
}

void ComponentSerializer::CopyAll(Entity source, Entity destination)
{
    for (auto& [id, metadata] : ComponentRegistry::GetRegistry())
    {
        if (metadata.Copy)
        {
            metadata.Copy(source, destination);
        }
    }
}

} // namespace Chained
