#include "component_registry.h"
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

    RegisterReflective<SpawnComponent>("SpawnZone", ICON_FA_LOCATION_DOT);
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
    {
        ComponentMetadata metadata;
        metadata.Name = "Widget";
        metadata.SerializationKey = "WidgetComponent";
        metadata.Icon = ICON_FA_WINDOW_RESTORE;
        metadata.Category = "UI";

        metadata.Has = [](Entity e) { return e.HasComponent<UIControlComponent>(); };
        metadata.GetAll = [](class Scene* s) {
            std::vector<uint64_t> ids;
            for (auto ent : s->GetRegistry().view<UIControlComponent>())
            {
                ids.push_back((uint64_t)(uint32_t)ent);
            }
            return ids;
        };
        metadata.Add = [](Entity e) {
            if (!e.HasComponent<UIControlComponent>())
            {
                e.AddComponent<UIControlComponent>();
            }
        };
        metadata.Remove = [](Entity e) {
            if (e.HasComponent<UIControlComponent>())
            {
                e.RemoveComponent<UIControlComponent>();
            }
        };
        metadata.Copy = [](Entity src, Entity dst) {
            if (src.HasComponent<UIControlComponent>())
            {
                dst.AddOrReplaceComponent<UIControlComponent>(src.GetComponent<UIControlComponent>());
            }
        };

        // Custom serialize — delegates to per-type Serialize methods
        metadata.Serialize = [](YAML::Emitter& out, Entity entity) {
            if (!entity.HasComponent<UIControlComponent>())
            {
                return;
            }
            auto& widgetComp = entity.GetComponent<UIControlComponent>();

            out << YAML::Key << "WidgetComponent" << YAML::Value << YAML::BeginMap;

            // Box Style
            out << YAML::Key << "Box Style" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "BG Color" << YAML::Value << widgetComp.BoxStyle.BackgroundColor;
            out << YAML::Key << "Hover Color" << YAML::Value << widgetComp.BoxStyle.HoverColor;
            out << YAML::Key << "Pressed Color" << YAML::Value << widgetComp.BoxStyle.PressedColor;
            out << YAML::Key << "Rounding" << YAML::Value << widgetComp.BoxStyle.Rounding;
            out << YAML::Key << "Border Size" << YAML::Value << widgetComp.BoxStyle.BorderSize;
            out << YAML::Key << "Border Color" << YAML::Value << widgetComp.BoxStyle.BorderColor;
            out << YAML::Key << "Gradient" << YAML::Value << widgetComp.BoxStyle.UseGradient;
            out << YAML::Key << "Padding" << YAML::Value << widgetComp.BoxStyle.Padding;
            out << YAML::Key << "Hover Scale" << YAML::Value << widgetComp.BoxStyle.HoverScale;
            out << YAML::Key << "Pressed Scale" << YAML::Value << widgetComp.BoxStyle.PressedScale;
            out << YAML::Key << "Transition Speed" << YAML::Value << widgetComp.BoxStyle.TransitionSpeed;
            out << YAML::EndMap;

            // Text Style
            out << YAML::Key << "Text Style" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Font Name" << YAML::Value << widgetComp.TextStyle.FontName;
            out << YAML::Key << "Font Size" << YAML::Value << widgetComp.TextStyle.FontSize;
            out << YAML::Key << "Text Color" << YAML::Value << widgetComp.TextStyle.TextColor;
            out << YAML::Key << "Shadow" << YAML::Value << widgetComp.TextStyle.Shadow;
            out << YAML::Key << "Letter Spacing" << YAML::Value << widgetComp.TextStyle.LetterSpacing;
            out << YAML::Key << "Line Height" << YAML::Value << widgetComp.TextStyle.LineHeight;
            out << YAML::Key << "H Align" << YAML::Value << static_cast<int>(widgetComp.TextStyle.Horizontal);
            out << YAML::Key << "V Align" << YAML::Value << static_cast<int>(widgetComp.TextStyle.Vertical);
            out << YAML::EndMap;

            // Widget data — delegate to per-type Serialize
            std::visit(
                [&](auto& data) {
                    using DataType = std::decay_t<decltype(data)>;
                    if constexpr (!std::is_same_v<DataType, std::monostate>)
                    {
                        data.Serialize(out);
                    }
                },
                widgetComp.Data);

            out << YAML::EndMap;
        };

        // Custom deserialize — delegates to DeserializeControlData
        metadata.Deserialize = [](Entity entity, YAML::Node node) {
            if (!node["WidgetComponent"])
            {
                return;
            }
            auto widgetNode = node["WidgetComponent"];
            auto& widgetComp = entity.AddComponent<UIControlComponent>();

            // Box Style
            if (auto boxStyleNode = widgetNode["Box Style"])
            {
                if (boxStyleNode["BG Color"])
                {
                    widgetComp.BoxStyle.BackgroundColor = boxStyleNode["BG Color"].as<Color>();
                }
                if (boxStyleNode["Hover Color"])
                {
                    widgetComp.BoxStyle.HoverColor = boxStyleNode["Hover Color"].as<Color>();
                }
                if (boxStyleNode["Pressed Color"])
                {
                    widgetComp.BoxStyle.PressedColor = boxStyleNode["Pressed Color"].as<Color>();
                }
                if (boxStyleNode["Rounding"])
                {
                    widgetComp.BoxStyle.Rounding = boxStyleNode["Rounding"].as<float>();
                }
                if (boxStyleNode["Border Size"])
                {
                    widgetComp.BoxStyle.BorderSize = boxStyleNode["Border Size"].as<float>();
                }
                if (boxStyleNode["Border Color"])
                {
                    widgetComp.BoxStyle.BorderColor = boxStyleNode["Border Color"].as<Color>();
                }
                if (boxStyleNode["Gradient"])
                {
                    widgetComp.BoxStyle.UseGradient = boxStyleNode["Gradient"].as<bool>();
                }
                if (boxStyleNode["Padding"])
                {
                    widgetComp.BoxStyle.Padding = boxStyleNode["Padding"].as<float>();
                }
                if (boxStyleNode["Hover Scale"])
                {
                    widgetComp.BoxStyle.HoverScale = boxStyleNode["Hover Scale"].as<float>();
                }
                if (boxStyleNode["Pressed Scale"])
                {
                    widgetComp.BoxStyle.PressedScale = boxStyleNode["Pressed Scale"].as<float>();
                }
                if (boxStyleNode["Transition Speed"])
                {
                    widgetComp.BoxStyle.TransitionSpeed = boxStyleNode["Transition Speed"].as<float>();
                }
            }

            // Text Style
            if (auto textStyleNode = widgetNode["Text Style"])
            {
                if (textStyleNode["Font Name"])
                {
                    widgetComp.TextStyle.FontName = textStyleNode["Font Name"].as<std::string>();
                }
                if (textStyleNode["Font Size"])
                {
                    widgetComp.TextStyle.FontSize = textStyleNode["Font Size"].as<float>();
                }
                if (textStyleNode["Text Color"])
                {
                    widgetComp.TextStyle.TextColor = textStyleNode["Text Color"].as<Color>();
                }
                if (textStyleNode["Shadow"])
                {
                    widgetComp.TextStyle.Shadow = textStyleNode["Shadow"].as<bool>();
                }
                if (textStyleNode["Letter Spacing"])
                {
                    widgetComp.TextStyle.LetterSpacing = textStyleNode["Letter Spacing"].as<float>();
                }
                if (textStyleNode["Line Height"])
                {
                    widgetComp.TextStyle.LineHeight = textStyleNode["Line Height"].as<float>();
                }
                if (textStyleNode["H Align"])
                {
                    widgetComp.TextStyle.Horizontal =
                        static_cast<HorizontalAlignment>(textStyleNode["H Align"].as<int>());
                }
                if (textStyleNode["V Align"])
                {
                    widgetComp.TextStyle.Vertical = static_cast<VerticalAlignment>(textStyleNode["V Align"].as<int>());
                }
            }

            // Widget data — delegate to DeserializeControlData
            int widgetType = widgetNode["Widget Type"] ? widgetNode["Widget Type"].as<int>() : 0;
            widgetComp.Data = DeserializeControlData(widgetType, widgetNode);
        };

        // Keep ReflectInternal for editor UI (property panel)
        metadata.IsReflective = true;
        metadata.ReflectInternal = [](Entity e, void* archivePtr, int mode) {
            IPropertyArchive* archive = static_cast<IPropertyArchive*>(archivePtr);
            const ReflectionMode reflMode = static_cast<ReflectionMode>(mode);
            if (reflMode == ReflectionMode::Deserialize)
            {
                auto& comp = e.AddOrReplaceComponent<UIControlComponent>();
                GenericProperties props(*archive);
                ReflectFromRfl(comp, props);
            }
            else if (e.HasComponent<UIControlComponent>())
            {
                GenericProperties props(*archive);
                auto& comp = e.GetComponent<UIControlComponent>();
                ReflectFromRfl(comp, props);
            }
        };

        metadata.GetSetField = [](Entity e, const std::string& fieldName, void* data, bool isSet) -> bool {
            bool found = false;
            auto& comp = e.GetComponent<UIControlComponent>();
            rfl::to_view(comp).apply([&](auto... field_pack) {
                (
                    [&](auto& field) {
                        if (found)
                        {
                            return;
                        }
                        std::string name(field.name());
                        if (name == fieldName)
                        {
                            using FieldType = std::decay_t<decltype(*field.get())>;
                            if (isSet)
                            {
                                *field.get() = *static_cast<FieldType*>(data);
                            }
                            else
                            {
                                *static_cast<FieldType*>(data) = *field.get();
                            }
                            found = true;
                        }
                    }(field_pack),
                    ...);
            });
            return found;
        };

        Register(entt::type_hash<UIControlComponent>::value(), metadata);
    }

    RegisterReflective<ControlComponent>("Control", nullptr, "UI");
    RegisterReflective<UIActionComponent>("UI Action", nullptr, "UI");
}
} // namespace Chained
