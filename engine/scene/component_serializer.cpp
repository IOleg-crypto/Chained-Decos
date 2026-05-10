#include "component_serializer.h"
#include "engine/scene/component_registry.h"
#include "engine/scene/scripting_serialization.h"
#include "engine/scene/hierarchy_serializer.h"
#include "components/id_component.h"
#include "components/ui_action_component.h"



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
