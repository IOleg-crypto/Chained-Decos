#include "component_serializer.h"
#include "components/control_component.h"
#include "components/id_component.h"
#include "components/ui_action_component.h"
#include "engine/scene/component_registry.h"
#include "engine/scene/hierarchy_serializer.h"
#include "engine/scene/scripting_serialization.h"

namespace CHEngine
{

ComponentSerializer::ComponentSerializer() = default;

void ComponentSerializer::OnInit()
{
    // Core components.
    Register<TagComponent>();
    Register<TransformComponent>();
    Register<ModelComponent>();
    Register<MaterialComponent>();
    Register<LightComponent>();
    Register<ShaderComponent>();
    Register<CameraComponent>();
    Register<SpriteComponent>();

    // Physics components.
    Register<ColliderComponent>();
    Register<PrimitiveComponent>();
    Register<RigidBodyComponent>();

    // Audio components.
    Register<AudioComponent>();

    // UI components.
    Register<ControlComponent>();
    Register<WidgetComponent>();
    Register<UIActionComponent>();

    // Managed scripts need a dedicated serializer because their payload is a list.
    ComponentMetadata metadata;
    metadata.Name = "Managed Script";
    metadata.SerializationKey = "ManagedScriptComponent";
    metadata.Category = "Scripting";
    metadata.Serialize = [](YAML::Emitter& out, Entity entity) {
        if (!entity.HasComponent<ManagedScriptComponent>())
        {
            return;
        }

        auto& comp = entity.GetComponent<ManagedScriptComponent>();
        out << YAML::Key << "ManagedScriptComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
        for (const auto& s : comp.Scripts)
        {
            out << s;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
    };
    metadata.Deserialize = [](Entity entity, YAML::Node node) {
        if (!node["ManagedScriptComponent"])
        {
            return;
        }

        if (!entity.HasComponent<ManagedScriptComponent>())
        {
            entity.AddComponent<ManagedScriptComponent>();
        }

        auto& comp = entity.GetComponent<ManagedScriptComponent>();
        comp.Scripts.clear();
        auto scriptsNode = node["ManagedScriptComponent"]["Scripts"];
        if (scriptsNode && scriptsNode.IsSequence())
        {
            for (auto s : scriptsNode)
            {
                CHEngine::ManagedScriptInstance inst;
                if (s.IsScalar())
                {
                    inst.ClassName = s.as<std::string>();
                }
                else
                {
                    YAML::convert<CHEngine::ManagedScriptInstance>::decode(s, inst);
                }
                comp.Scripts.push_back(std::move(inst));
            }
        }
    };
    metadata.Copy = [](Entity src, Entity dst) {
        if (src.HasComponent<ManagedScriptComponent>())
        {
            dst.AddOrReplaceComponent<ManagedScriptComponent>(src.GetComponent<ManagedScriptComponent>());
        }
    };
    metadata.Has = [](Entity e) { return e.HasComponent<ManagedScriptComponent>(); };
    metadata.GetAll = [](class Scene* s) {
        std::vector<uint64_t> ids;
        for (auto ent : s->GetRegistry().view<ManagedScriptComponent>())
        {
            ids.push_back((uint64_t)(uint32_t)ent);
        }
        return ids;
    };
    metadata.Add = [](Entity e) {
        if (!e.HasComponent<ManagedScriptComponent>())
        {
            e.AddComponent<ManagedScriptComponent>();
        }
    };
    metadata.Remove = [](Entity e) {
        if (e.HasComponent<ManagedScriptComponent>())
        {
            e.RemoveComponent<ManagedScriptComponent>();
        }
    };

    ComponentRegistry::Register(entt::type_hash<ManagedScriptComponent>::value(), metadata);
}

void ComponentSerializer::OnShutdown()
{
}

ComponentSerializer::~ComponentSerializer()
{
}

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

} // namespace CHEngine
