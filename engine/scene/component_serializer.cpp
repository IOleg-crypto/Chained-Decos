#include "component_serializer.h"
#include "engine/scene/component_registry.h"
#include "engine/scene/scripting_serialization.h"
#include "engine/scene/hierarchy_serializer.h"
#include "components/id_component.h"
#include "components/ui_action_component.h"
#include "components/control_component.h"



namespace CHEngine
{

ComponentSerializer::ComponentSerializer()
{
}

void ComponentSerializer::OnInit()
{
    m_Registry.clear();

    RegisterCoreComponents();
    RegisterPhysicsComponents();
    RegisterAudioComponents();
    RegisterUIComponents();
    RegisterScriptingComponents();
}

void ComponentSerializer::OnShutdown()
{
}

ComponentSerializer::~ComponentSerializer()
{
}

void ComponentSerializer::RegisterCustom(const ComponentSerializerEntry& entry)
{
    m_Registry.push_back(entry);

    // Bridge to the new ComponentRegistry metadata system
    // We try to find existing metadata; if not found, we creates a basic one.
    // We use a dummy type hash since we only have the string key in entry.
    // However, the Template Register<T> knows the type hash!
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


void ComponentSerializer::RegisterCoreComponents()
{
    Register<TagComponent>();
    Register<TransformComponent>();
    Register<ModelComponent>();
    Register<MaterialComponent>();
    Register<LightComponent>();
    Register<ShaderComponent>();
    Register<CameraComponent>();
    Register<SpriteComponent>();
}

void ComponentSerializer::RegisterPhysicsComponents()
{
    Register<ColliderComponent>();
    Register<PrimitiveComponent>();
    Register<RigidBodyComponent>();
}

void ComponentSerializer::RegisterAudioComponents()
{
    Register<AudioComponent>();
}


void ComponentSerializer::RegisterUIComponents()
{
    Register<ControlComponent>();
    Register<WidgetComponent>();
    Register<UIActionComponent>();

    // Legacy Support: Bridge old component names to unified WidgetComponent
    
    // 1. ButtonControl
    {
        ComponentSerializerEntry entry;
        entry.Key = "ButtonControl";
        entry.Deserialize = [](Entity entity, YAML::Node node) {
            if (node["ButtonControl"]) {
                auto& widget = entity.AddOrReplaceComponent<WidgetComponent>();
                ButtonData data;
                SerializationUtils::PropertyArchive archive(node["ButtonControl"]);
                CHEngine::Properties props(archive);
                props.Property("Label", data.Label);
                props.Property("Interactable", data.IsInteractable);
                props.Property("Auto Size", data.AutoSize);
                props.Nested("Text Style", widget.TextStyle);
                props.Nested("UI Style", widget.BoxStyle);
                widget.Data = data;
            }
        };
        RegisterCustom(entry);
    }

    // 2. LabelControl
    {
        ComponentSerializerEntry entry;
        entry.Key = "LabelControl";
        entry.Deserialize = [](Entity entity, YAML::Node node) {
            if (node["LabelControl"]) {
                auto& widget = entity.AddOrReplaceComponent<WidgetComponent>();
                LabelData data;
                SerializationUtils::PropertyArchive archive(node["LabelControl"]);
                CHEngine::Properties props(archive);
                props.Property("Text", data.Text);
                props.Property("Auto Size", data.AutoSize);
                props.Nested("Style", widget.TextStyle);
                widget.Data = data;
            }
        };
        RegisterCustom(entry);
    }

    // 3. ImageControl
    {
        ComponentSerializerEntry entry;
        entry.Key = "ImageControl";
        entry.Deserialize = [](Entity entity, YAML::Node node) {
            if (node["ImageControl"]) {
                auto& widget = entity.AddOrReplaceComponent<WidgetComponent>();
                ImageData data;
                SerializationUtils::PropertyArchive archive(node["ImageControl"]);
                CHEngine::Properties props(archive);
                props.Property("Texture Path", data.TexturePath);
                props.Property("Tint Color", data.TintColor);
                props.Property("Border Color", data.BorderColor);
                props.Nested("Style", widget.BoxStyle);
                widget.Data = data;
            }
        };
        RegisterCustom(entry);
    }
}

void ComponentSerializer::RegisterScriptingComponents()
{
    Register<ManagedScriptComponent>();
}

void ComponentSerializer::SerializeAll(YAML::Emitter& out, Entity entity)
{
    for (auto& [id, metadata] : ComponentRegistry::GetRegistry())
    {
        if (metadata.Serialize)
        {
            metadata.Serialize(out, entity);
        }
        else if (metadata.IsReflective && metadata.ReflectInternal)
        {
            SerializationUtils::PropertyArchive archive(out);
            metadata.ReflectInternal(entity, &archive, (int)ReflectionMode::Serialize);
        }
    }

    for (auto& entry : m_Registry)
    {
        if (entry.Serialize)
        {
            entry.Serialize(out, entity);
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
                SerializationUtils::PropertyArchive archive(node[metadata.SerializationKey]);
                metadata.ReflectInternal(entity, &archive, (int)ReflectionMode::Deserialize);
            }
        }
    }

    for (auto& entry : m_Registry)
    {
        if (entry.Deserialize)
        {
            entry.Deserialize(entity, node);
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

    for (auto& entry : m_Registry)
    {
        if (entry.Copy)
        {
            entry.Copy(source, destination);
        }
    }
}

} // namespace CHEngine
