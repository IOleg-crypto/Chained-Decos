#include "component_registry.h"
#include "components/control_component.h"
#include "components/scripting_components.h"
#include "engine/graphics/ui/ui_data_components.h"
#include "engine/graphics/ui/ui_style.h"
#include "engine/scene/yaml.h"
#include "thirdparty/IconsFontAwesome6.h"
#include <yaml-cpp/yaml.h>

namespace Chained
{
static void RegisterManagedScriptComponentMetadata()
{
    ComponentMetadata metadata;
    metadata.Name = "Managed Script";
    metadata.SerializationKey = "ManagedScriptComponent";
    metadata.Category = "Scripting";

    metadata.Serialize = [](YAML::Emitter& out, Entity entity) {
        if (!entity.HasComponent<ManagedScriptComponent>())
        {
            return;
        }

        auto& managedScript = entity.GetComponent<ManagedScriptComponent>();
        out << YAML::Key << "ManagedScriptComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;

        for (const auto& scriptInstance : managedScript.Scripts)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "ClassName" << YAML::Value << scriptInstance.ClassName;

            if (!scriptInstance.Fields.empty())
            {
                out << YAML::Key << "Fields" << YAML::Value << YAML::BeginSeq;
                for (const auto& [fieldName, field] : scriptInstance.Fields)
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "Name" << YAML::Value << fieldName;
                    out << YAML::Key << "Type" << YAML::Value << (int)field.Type;

                    std::visit(
                        [&out](auto&& fieldValue) {
                            using T = std::decay_t<decltype(fieldValue)>;
                            if constexpr (std::is_same_v<T, float> || std::is_same_v<T, int> ||
                                          std::is_same_v<T, bool> || std::is_same_v<T, std::string> ||
                                          std::is_same_v<T, uint64_t>)
                            {
                                out << YAML::Key << "Value" << YAML::Value << fieldValue;
                            }
                            else if constexpr (std::is_same_v<T, glm::vec2>)
                            {
                                out << YAML::Key << "Value" << YAML::Value << YAML::Flow << YAML::BeginSeq
                                    << fieldValue.x << fieldValue.y << YAML::EndSeq;
                            }
                            else if constexpr (std::is_same_v<T, glm::vec3>)
                            {
                                out << YAML::Key << "Value" << YAML::Value << YAML::Flow << YAML::BeginSeq
                                    << fieldValue.x << fieldValue.y << fieldValue.z << YAML::EndSeq;
                            }
                            else if constexpr (std::is_same_v<T, glm::vec4>)
                            {
                                out << YAML::Key << "Value" << YAML::Value << YAML::Flow << YAML::BeginSeq
                                    << fieldValue.x << fieldValue.y << fieldValue.z << fieldValue.w << YAML::EndSeq;
                            }
                            else if constexpr (std::is_same_v<T, Chained::Color>)
                            {
                                out << YAML::Key << "Value" << YAML::Value << YAML::Flow << YAML::BeginSeq
                                    << (int)fieldValue.r << (int)fieldValue.g << (int)fieldValue.b << (int)fieldValue.a
                                    << YAML::EndSeq;
                            }
                        },
                        field.Value);

                    out << YAML::EndMap;
                }
                out << YAML::EndSeq;
            }

            out << YAML::EndMap;
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

        auto& managedScript = entity.GetComponent<ManagedScriptComponent>();
        managedScript.Scripts.clear();

        auto scriptsNode = node["ManagedScriptComponent"]["Scripts"];
        if (!scriptsNode || !scriptsNode.IsSequence())
        {
            return;
        }

        for (auto scriptNode : scriptsNode)
        {
            ManagedScriptInstance scriptInstance;
            if (scriptNode["ClassName"])
            {
                scriptInstance.ClassName = scriptNode["ClassName"].as<std::string>();
            }

            if (scriptNode["Fields"] && scriptNode["Fields"].IsSequence())
            {
                for (auto fieldNode : scriptNode["Fields"])
                {
                    if (!fieldNode["Name"] || !fieldNode["Type"])
                    {
                        continue;
                    }

                    std::string fieldName = fieldNode["Name"].as<std::string>();
                    auto fieldType = (ScriptFieldType)fieldNode["Type"].as<int>(0);
                    auto valueNode = fieldNode["Value"];

                    ScriptField field;
                    field.Type = fieldType;
                    field.Name = fieldName;

                    if (valueNode)
                    {
                        switch (fieldType)
                        {
                        case ScriptFieldType::Float:
                            field.Value = valueNode.as<float>(0.0f);
                            break;
                        case ScriptFieldType::Int:
                            field.Value = valueNode.as<int>(0);
                            break;
                        case ScriptFieldType::Bool:
                            field.Value = valueNode.as<bool>(false);
                            break;
                        case ScriptFieldType::String:
                            field.Value = valueNode.as<std::string>("");
                            break;
                        case ScriptFieldType::Entity:
                            field.Value = valueNode.as<uint64_t>(0);
                            break;
                        case ScriptFieldType::Vec2:
                            field.Value = (valueNode.IsSequence() && valueNode.size() >= 2)
                                              ? glm::vec2(valueNode[0].as<float>(), valueNode[1].as<float>())
                                              : glm::vec2(0.0f);
                            break;
                        case ScriptFieldType::Vec3:
                            field.Value = (valueNode.IsSequence() && valueNode.size() >= 3)
                                              ? glm::vec3(valueNode[0].as<float>(), valueNode[1].as<float>(),
                                                          valueNode[2].as<float>())
                                              : glm::vec3(0.0f);
                            break;
                        case ScriptFieldType::Vec4:
                            field.Value = (valueNode.IsSequence() && valueNode.size() >= 4)
                                              ? glm::vec4(valueNode[0].as<float>(), valueNode[1].as<float>(),
                                                          valueNode[2].as<float>(), valueNode[3].as<float>())
                                              : glm::vec4(0.0f);
                            break;
                        case ScriptFieldType::Color:
                            field.Value = (valueNode.IsSequence() && valueNode.size() >= 4)
                                              ? Chained::Color{(unsigned char)valueNode[0].as<int>(),
                                                               (unsigned char)valueNode[1].as<int>(),
                                                               (unsigned char)valueNode[2].as<int>(),
                                                               (unsigned char)valueNode[3].as<int>()}
                                              : Chained::Color{255, 255, 255, 255};
                            break;
                        default:
                            field.Value = 0.0f;
                            break;
                        }
                    }

                    scriptInstance.Fields[fieldName] = std::move(field);
                }
            }

            managedScript.Scripts.push_back(std::move(scriptInstance));
        }
    };

    metadata.Copy = [](Entity src, Entity dst) {
        if (src.HasComponent<ManagedScriptComponent>())
        {
            dst.AddOrReplaceComponent<ManagedScriptComponent>(
                src.GetComponent<ManagedScriptComponent>().ClonePersistent());
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
    RegisterReflective<NameComponent>("Name", nullptr, "Core");
    RegisterReflective<HierarchyComponent>("Hierarchy", nullptr, "Core");

    // Graphics
    RegisterReflective<ModelComponent>("Model", ICON_FA_CUBE, "Rendering");
    RegisterReflective<LightComponent>("Light", ICON_FA_LIGHTBULB, "Rendering");
    RegisterReflective<SpriteComponent>("Sprite", ICON_FA_IMAGE, "Rendering");
    RegisterReflective<ShaderComponent>("Shader", nullptr, "Rendering");

    RegisterReflective<SpawnComponent>("Spawn", ICON_FA_LOCATION_DOT);
    RegisterReflective<PlayerComponent>("Player", ICON_FA_USER);

    // Audio
    RegisterReflective<AudioComponent>("Audio", ICON_FA_VOLUME_HIGH, "Audio");

    // Physics
    RegisterReflective<RigidBodyComponent>("Rigid Body", ICON_FA_CUBES, "Physics");
    RegisterReflective<ColliderComponent>("Collider", ICON_FA_SHIELD, "Physics");
    RegisterReflective<PrimitiveComponent>("Primitive", nullptr, "Physics");

    // Logic & Animation
    RegisterReflective<AnimationComponent>("Animation", ICON_FA_FILM, "Animation");
    RegisterReflective<SceneTransitionComponent>("Scene Transition", ICON_FA_DOOR_OPEN, "Gameplay");

    // Scripting
    RegisterManagedScriptComponentMetadata();

    // UI — custom serializer (bypasses broken variant reflection)
    extern void RegisterUIControlComponent();
    RegisterUIControlComponent();

    RegisterReflective<ControlComponent>("Control", nullptr, "UI");
    RegisterReflective<UIActionComponent>("UI Action", nullptr, "UI");
}
} // namespace Chained
