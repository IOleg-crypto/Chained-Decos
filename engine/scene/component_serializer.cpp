#include "component_serializer.h"
#include "components/hierarchy_component.h"
#include "components/id_component.h"
#include "components/ui_action_component.h"
#include "engine/core/application.h"

#include "scripting_serialization.h"

namespace CHEngine
{
static ComponentSerializer* s_Instance = nullptr;

ComponentSerializer::ComponentSerializer()
{
    CH_CORE_ASSERT(!s_Instance, "ComponentSerializer already exists!");
    s_Instance = this;
}

void ComponentSerializer::Init()
{
    if (!s_Instance)
    {
        s_Instance = new ComponentSerializer();
    }
    s_Instance->InternalInit();
}

void ComponentSerializer::Shutdown()
{
    if (s_Instance)
    {
        delete s_Instance;
        s_Instance = nullptr;
    }
}

ComponentSerializer::~ComponentSerializer()
{
    s_Instance = nullptr;
}

ComponentSerializer& ComponentSerializer::Get()
{
    CH_CORE_ASSERT(s_Instance, "ComponentSerializer not initialized!");
    return *s_Instance;
}

void ComponentSerializer::RegisterCustom(const ComponentSerializerEntry& entry)
{
    m_Registry.push_back(entry);
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

void ComponentSerializer::SerializeHierarchy(YAML::Emitter& out, Entity entity)
{
    if (entity.HasComponent<HierarchyComponent>())
    {
        auto& hc = entity.GetComponent<HierarchyComponent>();
        out << YAML::Key << "Hierarchy";
        out << YAML::BeginMap;

        uint64_t parentUUID = 0;
        if (hc.Parent != entt::null)
        {
            Entity parent{hc.Parent, &entity.GetRegistry()};
            if (parent.HasComponent<IDComponent>())
            {
                parentUUID = (uint64_t)parent.GetComponent<IDComponent>().ID;
            }
        }
        out << YAML::Key << "Parent" << YAML::Value << parentUUID;

        out << YAML::Key << "Children" << YAML::BeginSeq;
        for (auto childHandle : hc.Children)
        {
            Entity child{childHandle, &entity.GetRegistry()};
            if (child.HasComponent<IDComponent>())
            {
                out << (uint64_t)child.GetComponent<IDComponent>().ID;
            }
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }
}

void ComponentSerializer::DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask)
{
    if (node["Hierarchy"])
    {
        auto h = node["Hierarchy"];
        outTask.entity = entity;
        if (h["Parent"])
        {
            outTask.parent = h["Parent"].as<uint64_t>();
        }
        else
        {
            outTask.parent = 0;
        }

        if (h["Children"] && h["Children"].IsSequence())
        {
            for (auto child : h["Children"])
            {
                outTask.children.push_back(child.as<uint64_t>());
            }
        }
    }
}

// --- Registry Initialization ---

// ========================================================================
// Initialize Registry
// ========================================================================

void ComponentSerializer::InternalInit()
{
    m_Registry.clear();

    Register<TagComponent>();
    Register<TransformComponent>();
    Register<ModelComponent>();
    Register<MaterialComponent>();
    Register<LightComponent>();
    Register<ShaderComponent>();

    // --- Physics ---
    Register<ColliderComponent>();
    Register<PrimitiveComponent>();
    Register<RigidBodyComponent>();

    // --- Audio ---
    Register<AudioComponent>();
    Register<CameraComponent>();

    // --- Gameplay ---
    Register<PlayerComponent>();
    Register<SceneTransitionComponent>();
    Register<AnimationComponent>();
    Register<NavigationComponent>();
    Register<SpawnComponent>();
    Register<RPGStatsComponent>();
    Register<SkillComponent>();
    Register<InventoryComponent>();

    // --- Rendering ---
    Register<SpriteComponent>();

    // --- UI System ---
    Register<ControlComponent>();
    Register<ButtonControl>();
    Register<PanelControl>();
    Register<LabelControl>();
    Register<SliderControl>();
    Register<CheckboxControl>();
    Register<ImageControl>();
    Register<ImageButtonControl>();
    Register<InputTextControl>();
    Register<ComboBoxControl>();
    Register<ProgressBarControl>();
    Register<SeparatorControl>();
    Register<RadioButtonControl>();
    Register<ColorPickerControl>();
    Register<DragFloatControl>();
    Register<DragIntControl>();
    Register<TreeNodeControl>();
    Register<TabBarControl>();
    Register<TabItemControl>();
    Register<CollapsingHeaderControl>();
    Register<VerticalLayoutGroup>();
    Register<UIActionComponent>();

    // --- Scripting ---
    Register<ManagedScriptComponent>();
}

void ComponentSerializer::SerializeAll(YAML::Emitter& out, Entity entity)
{
    for (auto& entry : m_Registry)
    {
        entry.Serialize(out, entity);
    }
    SerializeHierarchy(out, entity);
}

void ComponentSerializer::DeserializeAll(Entity entity, YAML::Node node)
{
    for (const auto& entry : m_Registry)
    {
        if (entry.Deserialize)
        {
            entry.Deserialize(entity, node);
        }
    }
}

void ComponentSerializer::CopyAll(Entity source, Entity destination)
{
    for (const auto& entry : m_Registry)
    {
        if (entry.Copy)
        {
            entry.Copy(source, destination);
        }
    }
}

} // namespace CHEngine
