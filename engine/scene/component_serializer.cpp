#include "component_serializer.h"
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
        s_Instance->InternalShutdown();
        delete s_Instance;
        s_Instance = nullptr;
    }
}

ComponentSerializer::~ComponentSerializer()
{
    InternalShutdown();
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

// --- Registry Initialization ---

// ========================================================================
// Initialize Registry
// ========================================================================

void ComponentSerializer::InternalInit()
{
    if (m_Initialized)
    {
        return;
    }

    m_Registry.clear();

    RegisterCoreComponents();
    RegisterPhysicsComponents();
    RegisterAudioComponents();
    RegisterGameplayComponents();
    RegisterUIComponents();
    RegisterScriptingComponents();

    m_Initialized = true;
}

void ComponentSerializer::InternalShutdown()
{
    if (!m_Initialized)
    {
        return;
    }

    m_Registry.clear();
    m_Initialized = false;
}

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

void ComponentSerializer::RegisterGameplayComponents()
{
    Register<PlayerComponent>();
    Register<SceneTransitionComponent>();
    Register<AnimationComponent>();
    Register<NavigationComponent>();
    Register<SpawnComponent>();
    Register<RPGStatsComponent>();
    Register<SkillComponent>();
    Register<InventoryComponent>();
}

void ComponentSerializer::RegisterUIComponents()
{
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
}

void ComponentSerializer::RegisterScriptingComponents()
{
    Register<ManagedScriptComponent>();
}

void ComponentSerializer::SerializeAll(YAML::Emitter& out, Entity entity)
{
    for (auto& entry : m_Registry)
    {
        entry.Serialize(out, entity);
    }
    HierarchySerializer::Serialize(out, entity);
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
