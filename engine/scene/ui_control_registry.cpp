#include "component_registry.h"
#include "components/control_component.h"
#include "engine/graphics/ui/ui_data_components.h"
#include "engine/graphics/ui/ui_style.h"
#include "engine/scene/yaml.h"
#include "thirdparty/IconsFontAwesome6.h"
#include <yaml-cpp/yaml.h>

namespace Chained
{
void RegisterUIControlComponent()
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
        out << YAML::Key << "Gradient Color" << YAML::Value << widgetComp.BoxStyle.GradientColor;
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
        out << YAML::Key << "Shadow Offset" << YAML::Value << widgetComp.TextStyle.ShadowOffset;
        out << YAML::Key << "Shadow Color" << YAML::Value << widgetComp.TextStyle.ShadowColor;
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
            if (boxStyleNode["Gradient Color"])
            {
                widgetComp.BoxStyle.GradientColor = boxStyleNode["Gradient Color"].as<Color>();
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
            if (textStyleNode["Shadow Offset"])
            {
                widgetComp.TextStyle.ShadowOffset = textStyleNode["Shadow Offset"].as<float>();
            }
            if (textStyleNode["Shadow Color"])
            {
                widgetComp.TextStyle.ShadowColor = textStyleNode["Shadow Color"].as<Color>();
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
                widgetComp.TextStyle.Horizontal = static_cast<HorizontalAlignment>(textStyleNode["H Align"].as<int>());
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
        IPropertyArchiveBase* archive = static_cast<IPropertyArchiveBase*>(archivePtr);
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

    ComponentRegistry::Register(entt::type_hash<UIControlComponent>::value(), metadata);
}
} // namespace Chained
