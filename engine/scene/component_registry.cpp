#include "component_registry.h"
#include "components.h"
#include "IconsFontAwesome6.h"

namespace Chained
{
    std::unordered_map<::entt::id_type, ComponentMetadata> ComponentRegistry::s_Registry;

    void ComponentRegistry::Register(::entt::id_type typeId, const ComponentMetadata& metadata)
    {
        s_Registry[typeId] = metadata;
    }

    void ComponentRegistry::RegisterEngineComponents()
    {
        // Core
        RegisterReflective<TransformComponent>("Transform", ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, "Core");
        RegisterReflective<TagComponent>("Tag", ICON_FA_TAG, "Core");
        RegisterReflective<CameraComponent>("Camera", ICON_FA_VIDEO, "Core");
        RegisterReflective<IDComponent>("ID", nullptr, "Core"); // added ID
        
        // Graphics
        RegisterReflective<ModelComponent>("Model", ICON_FA_CUBE, "Graphics");
        RegisterReflective<LightComponent>("Light", ICON_FA_LIGHTBULB, "Graphics");
        RegisterReflective<SpriteComponent>("Sprite", ICON_FA_IMAGE, "Graphics");
        RegisterReflective<ShaderComponent>("Shader", nullptr, "Graphics"); // added Shader

        // Audio
        RegisterReflective<AudioComponent>("Audio", ICON_FA_VOLUME_HIGH, "Audio");

        // Physics
        RegisterReflective<RigidBodyComponent>("Rigid Body", ICON_FA_WEIGHT_HANGING, "Physics");
        RegisterReflective<ColliderComponent>("Collider", ICON_FA_SHIELD_HALVED, "Physics");
        RegisterReflective<PrimitiveComponent>("Primitive", nullptr, "Physics"); // added Primitive

        // Logic & Animation
        RegisterReflective<AnimationComponent>("Animation", ICON_FA_FILM, "Logic");
        RegisterReflective<SceneTransitionComponent>("Scene Transition", ICON_FA_DOOR_OPEN, "Logic");
        
        // UI
        RegisterReflective<UIControlComponent>("UI Control", ICON_FA_WINDOW_RESTORE, "UI");
        RegisterReflective<ControlComponent>("Control", nullptr, "UI");
        RegisterReflective<UIActionComponent>("UI Action", nullptr, "UI"); // added UI Action

        // Backward compatibility: old scene files used "FlowNavigatorComponent" (example)
        // ComponentRegistry::GetMetadataMutable(entt::type_hash<AnimationComponent>::value()).SerializationKey = "AnimationComponent";
    }
}

