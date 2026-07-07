#include "component_registry.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "engine/scene/managed_script_serializer.h"
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
    };

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

