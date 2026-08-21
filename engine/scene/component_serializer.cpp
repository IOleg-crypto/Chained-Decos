#include "component_serializer.h"
#include "components/core/id_component.h"
#include "engine/scene/hierarchy_serializer.h"
#include "engine/scene/serialization.h"

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
				metadata.ReflectInternal(entity, archive, ReflectionMode::Serialize);
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
				YAML::Node compNode = node[metadata.SerializationKey];
				if (!compNode)
				{
					// Fallback for scenes saved prior to space-stripping SerializationKey fix
					// e.g. "Scene TransitionComponent" or "Rigid BodyComponent"
					std::string legacyKey = metadata.Name + "Component";
					if (legacyKey != metadata.SerializationKey && node[legacyKey])
					{
						compNode = node[legacyKey];
					}
				}

				if (compNode)
				{
					Serialization::PropertyArchive archive(compNode);
					metadata.ReflectInternal(entity, archive, ReflectionMode::Deserialize);
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
