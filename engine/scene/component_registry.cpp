#include "component_registry.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "components/scripting_components.h"
#include "engine/graphics/ui/ui_data_components.h"
#include "engine/graphics/ui/ui_style.h"
#include <yaml-cpp/yaml.h>
#include "engine/scene/yaml.h"

namespace Chained
{
    // Helper for std::visit with multiple lambdas
    template <class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

    // Widget Type enum matching existing YAML format
    enum WidgetType : int
    {
        WidgetType_None = 0,
        WidgetType_Button = 1,
        WidgetType_Label = 3,
        WidgetType_Slider = 5,
        WidgetType_Checkbox = 6,
        WidgetType_ProgressBar = 7,
        WidgetType_Panel = 8,
        WidgetType_Image = 9,
        WidgetType_ComboBox = 10,
        WidgetType_ImageButton = 11,
        WidgetType_InputText = 12,
        WidgetType_Separator = 13,
        WidgetType_RadioButton = 14,
        WidgetType_ColorPicker = 15,
        WidgetType_DragFloat = 16,
        WidgetType_DragInt = 17,
        WidgetType_TreeNode = 18,
        WidgetType_TabBar = 19,
        WidgetType_TabItem = 20,
        WidgetType_CollapsingHeader = 21,
        WidgetType_PlotLines = 22,
        WidgetType_PlotHistogram = 23,
        WidgetType_VerticalLayoutGroup = 24,
    };

    static void RegisterManagedScriptComponentMetadata()
    {
        ComponentMetadata metadata;
        metadata.Name = "Managed Script";
        metadata.SerializationKey = "ManagedScriptComponent";
        metadata.Category = "Scripting";

        metadata.Serialize = [](YAML::Emitter& out, Entity entity) {
            if (!entity.HasComponent<ManagedScriptComponent>())
                return;

            auto& comp = entity.GetComponent<ManagedScriptComponent>();
            out << YAML::Key << "ManagedScriptComponent" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;

            for (const auto& s : comp.Scripts)
            {
                out << YAML::BeginMap;
                out << YAML::Key << "ClassName" << YAML::Value << s.ClassName;

                if (!s.Fields.empty())
                {
                    out << YAML::Key << "Fields" << YAML::Value << YAML::BeginSeq;
                    for (const auto& [fieldName, field] : s.Fields)
                    {
                        out << YAML::BeginMap;
                        out << YAML::Key << "Name"  << YAML::Value << fieldName;
                        out << YAML::Key << "Type"  << YAML::Value << (int)field.Type;

                        std::visit([&out](auto&& val) {
                            using T = std::decay_t<decltype(val)>;
                            if constexpr (std::is_same_v<T, float> || std::is_same_v<T, int> ||
                                          std::is_same_v<T, bool>  || std::is_same_v<T, std::string> ||
                                          std::is_same_v<T, uint64_t>)
                            {
                                out << YAML::Key << "Value" << YAML::Value << val;
                            }
                            else if constexpr (std::is_same_v<T, glm::vec2>)
                            {
                                out << YAML::Key << "Value" << YAML::Value
                                    << YAML::Flow << YAML::BeginSeq << val.x << val.y << YAML::EndSeq;
                            }
                            else if constexpr (std::is_same_v<T, glm::vec3>)
                            {
                                out << YAML::Key << "Value" << YAML::Value
                                    << YAML::Flow << YAML::BeginSeq << val.x << val.y << val.z << YAML::EndSeq;
                            }
                            else if constexpr (std::is_same_v<T, glm::vec4>)
                            {
                                out << YAML::Key << "Value" << YAML::Value
                                    << YAML::Flow << YAML::BeginSeq << val.x << val.y << val.z << val.w << YAML::EndSeq;
                            }
                            else if constexpr (std::is_same_v<T, Chained::Color>)
                            {
                                out << YAML::Key << "Value" << YAML::Value
                                    << YAML::Flow << YAML::BeginSeq
                                    << (int)val.r << (int)val.g << (int)val.b << (int)val.a
                                    << YAML::EndSeq;
                            }
                        }, field.Value);

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
                return;

            if (!entity.HasComponent<ManagedScriptComponent>())
                entity.AddComponent<ManagedScriptComponent>();

            auto& comp = entity.GetComponent<ManagedScriptComponent>();
            comp.Scripts.clear();

            auto scriptsNode = node["ManagedScriptComponent"]["Scripts"];
            if (!scriptsNode || !scriptsNode.IsSequence())
                return;

            for (auto s : scriptsNode)
            {
                ManagedScriptInstance inst;
                if (s["ClassName"])
                    inst.ClassName = s["ClassName"].as<std::string>();

                if (s["Fields"] && s["Fields"].IsSequence())
                {
                    for (auto fieldNode : s["Fields"])
                    {
                        if (!fieldNode["Name"] || !fieldNode["Type"])
                            continue;

                        std::string name   = fieldNode["Name"].as<std::string>();
                        auto        type   = (ScriptFieldType)fieldNode["Type"].as<int>(0);
                        auto        valN   = fieldNode["Value"];

                        ScriptField field;
                        field.Type = type;
                        field.Name = name;

                        if (valN)
                        {
                            switch (type)
                            {
                                case ScriptFieldType::Float:
                                    field.Value = valN.as<float>(0.0f);     break;
                                case ScriptFieldType::Int:
                                    field.Value = valN.as<int>(0);          break;
                                case ScriptFieldType::Bool:
                                    field.Value = valN.as<bool>(false);     break;
                                case ScriptFieldType::String:
                                    field.Value = valN.as<std::string>(""); break;
                                case ScriptFieldType::Entity:
                                    field.Value = valN.as<uint64_t>(0);     break;
                                case ScriptFieldType::Vec2:
                                    field.Value = (valN.IsSequence() && valN.size() >= 2)
                                        ? glm::vec2(valN[0].as<float>(), valN[1].as<float>())
                                        : glm::vec2(0.0f);
                                    break;
                                case ScriptFieldType::Vec3:
                                    field.Value = (valN.IsSequence() && valN.size() >= 3)
                                        ? glm::vec3(valN[0].as<float>(), valN[1].as<float>(), valN[2].as<float>())
                                        : glm::vec3(0.0f);
                                    break;
                                case ScriptFieldType::Vec4:
                                    field.Value = (valN.IsSequence() && valN.size() >= 4)
                                        ? glm::vec4(valN[0].as<float>(), valN[1].as<float>(),
                                                    valN[2].as<float>(), valN[3].as<float>())
                                        : glm::vec4(0.0f);
                                    break;
                                case ScriptFieldType::Color:
                                    field.Value = (valN.IsSequence() && valN.size() >= 4)
                                        ? Chained::Color{(unsigned char)valN[0].as<int>(),
                                                         (unsigned char)valN[1].as<int>(),
                                                         (unsigned char)valN[2].as<int>(),
                                                         (unsigned char)valN[3].as<int>()}
                                        : Chained::Color{255, 255, 255, 255};
                                    break;
                                default:
                                    field.Value = 0.0f; break;
                            }
                        }

                        inst.Fields[name] = std::move(field);
                    }
                }

                comp.Scripts.push_back(std::move(inst));
            }
        };

        metadata.Copy = [](Entity src, Entity dst) {
            if (src.HasComponent<ManagedScriptComponent>())
                dst.AddOrReplaceComponent<ManagedScriptComponent>(
                    src.GetComponent<ManagedScriptComponent>().ClonePersistent());
        };
        metadata.Has    = [](Entity e)       { return e.HasComponent<ManagedScriptComponent>(); };
        metadata.GetAll = [](class Scene* s) {
            std::vector<uint64_t> ids;
            for (auto ent : s->GetRegistry().view<ManagedScriptComponent>())
                ids.push_back((uint64_t)(uint32_t)ent);
            return ids;
        };
        metadata.Add    = [](Entity e) {
            if (!e.HasComponent<ManagedScriptComponent>())
                e.AddComponent<ManagedScriptComponent>();
        };
        metadata.Remove = [](Entity e) {
            if (e.HasComponent<ManagedScriptComponent>())
                e.RemoveComponent<ManagedScriptComponent>();
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
                    ids.push_back((uint64_t)(uint32_t)ent);
                return ids;
            };
            metadata.Add = [](Entity e) { if (!e.HasComponent<UIControlComponent>()) e.AddComponent<UIControlComponent>(); };
            metadata.Remove = [](Entity e) { if (e.HasComponent<UIControlComponent>()) e.RemoveComponent<UIControlComponent>(); };
            metadata.Copy = [](Entity src, Entity dst) {
                if (src.HasComponent<UIControlComponent>())
                    dst.AddOrReplaceComponent<UIControlComponent>(src.GetComponent<UIControlComponent>());
            };

            // Custom serialize — writes BoxStyle, TextStyle, WidgetType, variant fields
            metadata.Serialize = [](YAML::Emitter& out, Entity entity) {
                if (!entity.HasComponent<UIControlComponent>()) return;
                auto& comp = entity.GetComponent<UIControlComponent>();

                out << YAML::Key << "WidgetComponent" << YAML::Value << YAML::BeginMap;

                // Box Style
                out << YAML::Key << "Box Style" << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "BG Color" << YAML::Value << comp.BoxStyle.BackgroundColor;
                out << YAML::Key << "Hover Color" << YAML::Value << comp.BoxStyle.HoverColor;
                out << YAML::Key << "Pressed Color" << YAML::Value << comp.BoxStyle.PressedColor;
                out << YAML::Key << "Rounding" << YAML::Value << comp.BoxStyle.Rounding;
                out << YAML::Key << "Border Size" << YAML::Value << comp.BoxStyle.BorderSize;
                out << YAML::Key << "Border Color" << YAML::Value << comp.BoxStyle.BorderColor;
                out << YAML::Key << "Gradient" << YAML::Value << comp.BoxStyle.UseGradient;
                out << YAML::Key << "Padding" << YAML::Value << comp.BoxStyle.Padding;
                out << YAML::Key << "Hover Scale" << YAML::Value << comp.BoxStyle.HoverScale;
                out << YAML::Key << "Pressed Scale" << YAML::Value << comp.BoxStyle.PressedScale;
                out << YAML::Key << "Transition Speed" << YAML::Value << comp.BoxStyle.TransitionSpeed;
                out << YAML::EndMap;

                // Text Style
                out << YAML::Key << "Text Style" << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "Font Name" << YAML::Value << comp.TextStyle.FontName;
                out << YAML::Key << "Font Size" << YAML::Value << comp.TextStyle.FontSize;
                out << YAML::Key << "Text Color" << YAML::Value << comp.TextStyle.TextColor;
                out << YAML::Key << "Shadow" << YAML::Value << comp.TextStyle.Shadow;
                out << YAML::Key << "Letter Spacing" << YAML::Value << comp.TextStyle.LetterSpacing;
                out << YAML::Key << "Line Height" << YAML::Value << comp.TextStyle.LineHeight;
                out << YAML::Key << "H Align" << YAML::Value << static_cast<int>(comp.TextStyle.Horizontal);
                out << YAML::Key << "V Align" << YAML::Value << static_cast<int>(comp.TextStyle.Vertical);
                out << YAML::EndMap;

                // Widget Type + variant-specific fields
                std::visit(Overloaded{
                    [&](std::monostate) {},
                    [&](ButtonData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_Button;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Interactable" << YAML::Value << d.IsInteractable;
                        out << YAML::Key << "Auto Size" << YAML::Value << d.AutoSize;
                    },
                    [&](LabelData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_Label;
                        out << YAML::Key << "Text" << YAML::Value << d.Text;
                        out << YAML::Key << "Auto Size" << YAML::Value << d.AutoSize;
                    },
                    [&](SliderData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_Slider;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Value" << YAML::Value << d.Value;
                        out << YAML::Key << "Min" << YAML::Value << d.Min;
                        out << YAML::Key << "Max" << YAML::Value << d.Max;
                    },
                    [&](CheckboxData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_Checkbox;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Checked" << YAML::Value << d.Checked;
                    },
                    [&](ProgressBarData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_ProgressBar;
                        out << YAML::Key << "Progress" << YAML::Value << d.Progress;
                        out << YAML::Key << "Overlay Text" << YAML::Value << d.OverlayText;
                        out << YAML::Key << "Show Percentage" << YAML::Value << d.ShowPercentage;
                    },
                    [&](PanelData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_Panel;
                        out << YAML::Key << "Texture Path" << YAML::Value << d.TexturePath;
                        out << YAML::Key << "Full Screen" << YAML::Value << d.FullScreen;
                    },
                    [&](ImageData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_Image;
                        out << YAML::Key << "Texture Path" << YAML::Value << d.TexturePath;
                        out << YAML::Key << "Tint Color" << YAML::Value << d.TintColor;
                        out << YAML::Key << "Border Color" << YAML::Value << d.BorderColor;
                    },
                    [&](ComboBoxData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_ComboBox;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Items" << YAML::Value << YAML::BeginSeq;
                        for (auto& item : d.Items) out << item;
                        out << YAML::EndSeq;
                        out << YAML::Key << "Selected Index" << YAML::Value << d.SelectedIndex;
                    },
                    [&](ImageButtonData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_ImageButton;
                        out << YAML::Key << "Texture Path" << YAML::Value << d.TexturePath;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Tint Color" << YAML::Value << d.TintColor;
                        out << YAML::Key << "Background Color" << YAML::Value << d.BackgroundColor;
                        out << YAML::Key << "Frame Padding" << YAML::Value << d.FramePadding;
                    },
                    [&](InputTextData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_InputText;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Text" << YAML::Value << d.Text;
                        out << YAML::Key << "Placeholder" << YAML::Value << d.Placeholder;
                        out << YAML::Key << "Max Length" << YAML::Value << d.MaxLength;
                        out << YAML::Key << "Multiline" << YAML::Value << d.Multiline;
                        out << YAML::Key << "Read Only" << YAML::Value << d.ReadOnly;
                        out << YAML::Key << "Password" << YAML::Value << d.Password;
                    },
                    [&](SeparatorData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_Separator;
                        out << YAML::Key << "Thickness" << YAML::Value << d.Thickness;
                        out << YAML::Key << "Line Color" << YAML::Value << d.LineColor;
                    },
                    [&](RadioButtonData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_RadioButton;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Options" << YAML::Value << YAML::BeginSeq;
                        for (auto& opt : d.Options) out << opt;
                        out << YAML::EndSeq;
                        out << YAML::Key << "Selected Index" << YAML::Value << d.SelectedIndex;
                        out << YAML::Key << "Horizontal" << YAML::Value << d.Horizontal;
                    },
                    [&](ColorPickerData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_ColorPicker;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Selected Color" << YAML::Value << d.SelectedColor;
                        out << YAML::Key << "Show Alpha" << YAML::Value << d.ShowAlpha;
                        out << YAML::Key << "Show Picker" << YAML::Value << d.ShowPicker;
                    },
                    [&](DragFloatData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_DragFloat;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Value" << YAML::Value << d.Value;
                        out << YAML::Key << "Speed" << YAML::Value << d.Speed;
                        out << YAML::Key << "Min" << YAML::Value << d.Min;
                        out << YAML::Key << "Max" << YAML::Value << d.Max;
                        out << YAML::Key << "Format" << YAML::Value << d.Format;
                    },
                    [&](DragIntData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_DragInt;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Value" << YAML::Value << d.Value;
                        out << YAML::Key << "Speed" << YAML::Value << d.Speed;
                        out << YAML::Key << "Min" << YAML::Value << d.Min;
                        out << YAML::Key << "Max" << YAML::Value << d.Max;
                        out << YAML::Key << "Format" << YAML::Value << d.Format;
                    },
                    [&](TreeNodeData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_TreeNode;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Is Open" << YAML::Value << d.IsOpen;
                        out << YAML::Key << "Default Open" << YAML::Value << d.DefaultOpen;
                        out << YAML::Key << "Is Leaf" << YAML::Value << d.IsLeaf;
                    },
                    [&](TabBarData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_TabBar;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Reorderable" << YAML::Value << d.Reorderable;
                        out << YAML::Key << "Auto Select New Tabs" << YAML::Value << d.AutoSelectNewTabs;
                    },
                    [&](TabItemData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_TabItem;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Is Open" << YAML::Value << d.IsOpen;
                        out << YAML::Key << "Selected" << YAML::Value << d.Selected;
                    },
                    [&](CollapsingHeaderData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_CollapsingHeader;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Is Open" << YAML::Value << d.IsOpen;
                        out << YAML::Key << "Default Open" << YAML::Value << d.DefaultOpen;
                    },
                    [&](PlotLinesData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_PlotLines;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Values" << YAML::Value << YAML::BeginSeq;
                        for (auto v : d.Values) out << v;
                        out << YAML::EndSeq;
                        out << YAML::Key << "Overlay Text" << YAML::Value << d.OverlayText;
                        out << YAML::Key << "Scale Min" << YAML::Value << d.ScaleMin;
                        out << YAML::Key << "Scale Max" << YAML::Value << d.ScaleMax;
                    },
                    [&](PlotHistogramData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_PlotHistogram;
                        out << YAML::Key << "Label" << YAML::Value << d.Label;
                        out << YAML::Key << "Values" << YAML::Value << YAML::BeginSeq;
                        for (auto v : d.Values) out << v;
                        out << YAML::EndSeq;
                        out << YAML::Key << "Overlay Text" << YAML::Value << d.OverlayText;
                        out << YAML::Key << "Scale Min" << YAML::Value << d.ScaleMin;
                        out << YAML::Key << "Scale Max" << YAML::Value << d.ScaleMax;
                    },
                    [&](VerticalLayoutGroupData& d) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_VerticalLayoutGroup;
                        out << YAML::Key << "Spacing" << YAML::Value << d.Spacing;
                        out << YAML::Key << "Padding" << YAML::Value << d.Padding;
                    },
                    [&](auto&) {
                        out << YAML::Key << "Widget Type" << YAML::Value << WidgetType_None;
                    }
                }, comp.Data);

                out << YAML::EndMap;
            };

            // Custom deserialize — reads existing YAML format
            metadata.Deserialize = [](Entity entity, YAML::Node node) {
                if (!node["WidgetComponent"]) return;
                auto w = node["WidgetComponent"];
                auto& comp = entity.AddComponent<UIControlComponent>();

                // Box Style
                if (w["Box Style"]) {
                    auto bs = w["Box Style"];
                    if (bs["BG Color"]) comp.BoxStyle.BackgroundColor = bs["BG Color"].as<Color>();
                    if (bs["Hover Color"]) comp.BoxStyle.HoverColor = bs["Hover Color"].as<Color>();
                    if (bs["Pressed Color"]) comp.BoxStyle.PressedColor = bs["Pressed Color"].as<Color>();
                    if (bs["Rounding"]) comp.BoxStyle.Rounding = bs["Rounding"].as<float>();
                    if (bs["Border Size"]) comp.BoxStyle.BorderSize = bs["Border Size"].as<float>();
                    if (bs["Border Color"]) comp.BoxStyle.BorderColor = bs["Border Color"].as<Color>();
                    if (bs["Gradient"]) comp.BoxStyle.UseGradient = bs["Gradient"].as<bool>();
                    if (bs["Padding"]) comp.BoxStyle.Padding = bs["Padding"].as<float>();
                    if (bs["Hover Scale"]) comp.BoxStyle.HoverScale = bs["Hover Scale"].as<float>();
                    if (bs["Pressed Scale"]) comp.BoxStyle.PressedScale = bs["Pressed Scale"].as<float>();
                    if (bs["Transition Speed"]) comp.BoxStyle.TransitionSpeed = bs["Transition Speed"].as<float>();
                }

                // Text Style
                if (w["Text Style"]) {
                    auto ts = w["Text Style"];
                    if (ts["Font Name"]) comp.TextStyle.FontName = ts["Font Name"].as<std::string>();
                    if (ts["Font Size"]) comp.TextStyle.FontSize = ts["Font Size"].as<float>();
                    if (ts["Text Color"]) comp.TextStyle.TextColor = ts["Text Color"].as<Color>();
                    if (ts["Shadow"]) comp.TextStyle.Shadow = ts["Shadow"].as<bool>();
                    if (ts["Letter Spacing"]) comp.TextStyle.LetterSpacing = ts["Letter Spacing"].as<float>();
                    if (ts["Line Height"]) comp.TextStyle.LineHeight = ts["Line Height"].as<float>();
                    if (ts["H Align"]) comp.TextStyle.Horizontal = static_cast<HorizontalAlignment>(ts["H Align"].as<int>());
                    if (ts["V Align"]) comp.TextStyle.Vertical = static_cast<VerticalAlignment>(ts["V Align"].as<int>());
                }

                // Widget Type + variant fields
                int widgetType = w["Widget Type"] ? w["Widget Type"].as<int>() : 0;
                switch (widgetType) {
                    case WidgetType_Button: {
                        ButtonData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Interactable"]) d.IsInteractable = w["Interactable"].as<bool>();
                        if (w["Auto Size"]) d.AutoSize = w["Auto Size"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_Label: {
                        LabelData d;
                        if (w["Text"]) d.Text = w["Text"].as<std::string>();
                        if (w["Auto Size"]) d.AutoSize = w["Auto Size"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_Slider: {
                        SliderData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Value"]) d.Value = w["Value"].as<float>();
                        if (w["Min"]) d.Min = w["Min"].as<float>();
                        if (w["Max"]) d.Max = w["Max"].as<float>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_Checkbox: {
                        CheckboxData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Checked"]) d.Checked = w["Checked"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_ProgressBar: {
                        ProgressBarData d;
                        if (w["Progress"]) d.Progress = w["Progress"].as<float>();
                        if (w["Overlay Text"]) d.OverlayText = w["Overlay Text"].as<std::string>();
                        if (w["Show Percentage"]) d.ShowPercentage = w["Show Percentage"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_Panel: {
                        PanelData d;
                        if (w["Texture Path"]) d.TexturePath = w["Texture Path"].as<std::string>();
                        if (w["Full Screen"]) d.FullScreen = w["Full Screen"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_Image: {
                        ImageData d;
                        if (w["Texture Path"]) d.TexturePath = w["Texture Path"].as<std::string>();
                        if (w["Tint Color"]) d.TintColor = w["Tint Color"].as<Color>();
                        if (w["Border Color"]) d.BorderColor = w["Border Color"].as<Color>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_ComboBox: {
                        ComboBoxData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Items"]) for (auto item : w["Items"]) d.Items.push_back(item.as<std::string>());
                        if (w["Selected Index"]) d.SelectedIndex = w["Selected Index"].as<int>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_ImageButton: {
                        ImageButtonData d;
                        if (w["Texture Path"]) d.TexturePath = w["Texture Path"].as<std::string>();
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Tint Color"]) d.TintColor = w["Tint Color"].as<Color>();
                        if (w["Background Color"]) d.BackgroundColor = w["Background Color"].as<Color>();
                        if (w["Frame Padding"]) d.FramePadding = w["Frame Padding"].as<int>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_InputText: {
                        InputTextData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Text"]) d.Text = w["Text"].as<std::string>();
                        if (w["Placeholder"]) d.Placeholder = w["Placeholder"].as<std::string>();
                        if (w["Max Length"]) d.MaxLength = w["Max Length"].as<int>();
                        if (w["Multiline"]) d.Multiline = w["Multiline"].as<bool>();
                        if (w["Read Only"]) d.ReadOnly = w["Read Only"].as<bool>();
                        if (w["Password"]) d.Password = w["Password"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_Separator: {
                        SeparatorData d;
                        if (w["Thickness"]) d.Thickness = w["Thickness"].as<float>();
                        if (w["Line Color"]) d.LineColor = w["Line Color"].as<Color>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_RadioButton: {
                        RadioButtonData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Options"]) for (auto opt : w["Options"]) d.Options.push_back(opt.as<std::string>());
                        if (w["Selected Index"]) d.SelectedIndex = w["Selected Index"].as<int>();
                        if (w["Horizontal"]) d.Horizontal = w["Horizontal"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_ColorPicker: {
                        ColorPickerData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Selected Color"]) d.SelectedColor = w["Selected Color"].as<Color>();
                        if (w["Show Alpha"]) d.ShowAlpha = w["Show Alpha"].as<bool>();
                        if (w["Show Picker"]) d.ShowPicker = w["Show Picker"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_DragFloat: {
                        DragFloatData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Value"]) d.Value = w["Value"].as<float>();
                        if (w["Speed"]) d.Speed = w["Speed"].as<float>();
                        if (w["Min"]) d.Min = w["Min"].as<float>();
                        if (w["Max"]) d.Max = w["Max"].as<float>();
                        if (w["Format"]) d.Format = w["Format"].as<std::string>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_DragInt: {
                        DragIntData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Value"]) d.Value = w["Value"].as<int>();
                        if (w["Speed"]) d.Speed = w["Speed"].as<float>();
                        if (w["Min"]) d.Min = w["Min"].as<int>();
                        if (w["Max"]) d.Max = w["Max"].as<int>();
                        if (w["Format"]) d.Format = w["Format"].as<std::string>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_TreeNode: {
                        TreeNodeData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Is Open"]) d.IsOpen = w["Is Open"].as<bool>();
                        if (w["Default Open"]) d.DefaultOpen = w["Default Open"].as<bool>();
                        if (w["Is Leaf"]) d.IsLeaf = w["Is Leaf"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_TabBar: {
                        TabBarData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Reorderable"]) d.Reorderable = w["Reorderable"].as<bool>();
                        if (w["Auto Select New Tabs"]) d.AutoSelectNewTabs = w["Auto Select New Tabs"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_TabItem: {
                        TabItemData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Is Open"]) d.IsOpen = w["Is Open"].as<bool>();
                        if (w["Selected"]) d.Selected = w["Selected"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_CollapsingHeader: {
                        CollapsingHeaderData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Is Open"]) d.IsOpen = w["Is Open"].as<bool>();
                        if (w["Default Open"]) d.DefaultOpen = w["Default Open"].as<bool>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_PlotLines: {
                        PlotLinesData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Values"]) for (auto v : w["Values"]) d.Values.push_back(v.as<float>());
                        if (w["Overlay Text"]) d.OverlayText = w["Overlay Text"].as<std::string>();
                        if (w["Scale Min"]) d.ScaleMin = w["Scale Min"].as<float>();
                        if (w["Scale Max"]) d.ScaleMax = w["Scale Max"].as<float>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_PlotHistogram: {
                        PlotHistogramData d;
                        if (w["Label"]) d.Label = w["Label"].as<std::string>();
                        if (w["Values"]) for (auto v : w["Values"]) d.Values.push_back(v.as<float>());
                        if (w["Overlay Text"]) d.OverlayText = w["Overlay Text"].as<std::string>();
                        if (w["Scale Min"]) d.ScaleMin = w["Scale Min"].as<float>();
                        if (w["Scale Max"]) d.ScaleMax = w["Scale Max"].as<float>();
                        comp.Data = d;
                        break;
                    }
                    case WidgetType_VerticalLayoutGroup: {
                        VerticalLayoutGroupData d;
                        if (w["Spacing"]) d.Spacing = w["Spacing"].as<float>();
                        if (w["Padding"]) d.Padding = w["Padding"].as<glm::vec2>();
                        comp.Data = d;
                        break;
                    }
                    default:
                        break;
                }
            };

            // Keep ReflectInternal for editor UI (property panel)
            metadata.IsReflective = true;
            metadata.ReflectInternal = [](Entity e, void* archivePtr, int mode) {
                IPropertyArchive* archive = static_cast<IPropertyArchive*>(archivePtr);
                const ReflectionMode reflMode = static_cast<ReflectionMode>(mode);
                if (reflMode == ReflectionMode::Deserialize) {
                    auto& comp = e.AddOrReplaceComponent<UIControlComponent>();
                    GenericProperties props(*archive);
                    ReflectFromRfl(comp, props);
                } else if (e.HasComponent<UIControlComponent>()) {
                    GenericProperties props(*archive);
                    auto& comp = e.GetComponent<UIControlComponent>();
                    ReflectFromRfl(comp, props);
                }
            };

            metadata.GetSetField = [](Entity e, const std::string& fieldName, void* data, bool isSet) -> bool {
                bool found = false;
                auto& comp = e.GetComponent<UIControlComponent>();
                rfl::to_view(comp).apply([&](auto... field_pack) {
                    ([&](auto& field) {
                        if (found) return;
                        std::string name(field.name());
                        if (name == fieldName) {
                            using FieldType = std::decay_t<decltype(*field.get())>;
                            if (isSet) *field.get() = *static_cast<FieldType*>(data);
                            else *static_cast<FieldType*>(data) = *field.get();
                            found = true;
                        }
                    }(field_pack), ...);
                });
                return found;
            };

            Register(entt::type_hash<UIControlComponent>::value(), metadata);
        }

        RegisterReflective<ControlComponent>("Control", nullptr, "UI");
        RegisterReflective<UIActionComponent>("UI Action", nullptr, "UI");

        // Scripting
        RegisterManagedScriptComponentMetadata();
    }
}

