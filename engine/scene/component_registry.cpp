#include "component_registry.h"
#include "components.h"
#include "IconsFontAwesome6.h"

namespace CHEngine
{
    std::unordered_map<::entt::id_type, ComponentMetadata> ComponentRegistry::s_Registry;

    void ComponentRegistry::Register(::entt::id_type typeId, const ComponentMetadata& metadata)
    {
        s_Registry[typeId] = metadata;
    }

    void ComponentRegistry::RegisterEngineComponents()
    {
        RegisterReflective<TransformComponent>("Transform", ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, "Core");
        RegisterReflective<TagComponent>("Tag", ICON_FA_TAG, "Core");
        RegisterReflective<CameraComponent>("Camera", ICON_FA_VIDEO, "Core");
        RegisterReflective<ModelComponent>("Mesh", ICON_FA_CUBE, "Graphics");
        RegisterReflective<MaterialComponent>("Material Overrides", ICON_FA_FILL_DRIP, "Graphics");
        RegisterReflective<LightComponent>("Light", ICON_FA_LIGHTBULB, "Graphics");
        RegisterReflective<SpriteComponent>("Sprite", ICON_FA_IMAGE, "Graphics");
        RegisterReflective<AudioComponent>("Audio", ICON_FA_VOLUME_HIGH, "Audio");
        RegisterReflective<RigidBodyComponent>("Rigid Body", ICON_FA_WEIGHT_HANGING, "Physics");
        RegisterReflective<ColliderComponent>("Collider", ICON_FA_SHIELD_HALVED, "Physics");
        RegisterReflective<AnimationComponent>("Animation", ICON_FA_FILM, "Logic");
        // Backward compatibility: old scene files used "FlowNavigatorComponent"
        ComponentRegistry::GetMetadataMutable(entt::type_hash<AnimationComponent>::value()).SerializationKey = "AnimationComponent";
        RegisterReflective<WidgetComponent>("Widget", ICON_FA_WINDOW_RESTORE, "UI");
    }
}
