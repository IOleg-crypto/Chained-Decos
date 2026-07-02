#include "component_registry.h"
#include "components.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "engine/serialization/managed_script_serializer.h"

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
        RegisterReflective<IDComponent>("ID", nullptr, "Core");
        
        // Graphics
        RegisterReflective<ModelComponent>("Model", ICON_FA_CUBE, "Graphics");
        RegisterReflective<LightComponent>("Light", ICON_FA_LIGHTBULB, "Graphics");
        RegisterReflective<SpriteComponent>("Sprite", ICON_FA_IMAGE, "Graphics");
        RegisterReflective<ShaderComponent>("Shader", nullptr, "Graphics");

        // Audio
        RegisterReflective<AudioComponent>("Audio", ICON_FA_VOLUME_HIGH, "Audio");

        // Physics
        RegisterReflective<RigidBodyComponent>("Rigid Body", ICON_FA_CUBES, "Physics");
        RegisterReflective<ColliderComponent>("Collider", ICON_FA_SHIELD, "Physics");
        RegisterReflective<PrimitiveComponent>("Primitive", nullptr, "Physics");

        // Logic & Animation
        RegisterReflective<AnimationComponent>("Animation", ICON_FA_FILM, "Logic");
        RegisterReflective<SceneTransitionComponent>("Scene Transition", ICON_FA_DOOR_OPEN, "Logic");
        
        // UI
        RegisterReflective<UIControlComponent>("UI Control", ICON_FA_WINDOW_RESTORE, "UI");
        RegisterReflective<ControlComponent>("Control", nullptr, "UI");
        RegisterReflective<UIActionComponent>("UI Action", nullptr, "UI");

        // Scripting — must be registered here so scripts are serialized and deserialized correctly
        RegisterManagedScriptComponentMetadata();
    }
}

