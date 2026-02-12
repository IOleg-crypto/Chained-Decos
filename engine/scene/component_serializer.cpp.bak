#include "component_serializer.h"
#include "scene.h"
#include "components.h"
#include "engine/core/yaml.h"
#include "script_registry.h"
#include "components/hierarchy_component.h"
#include "components/id_component.h"
#include "engine/scene/serialization_utils.h"

namespace CHEngine
{
    using namespace SerializationUtils;

    std::vector<ComponentSerializerEntry> ComponentSerializer::s_Registry;

    // --- Special Serialization Helpers ---

    void ComponentSerializer::SerializeID(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<IDComponent>())
            out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entity.GetComponent<IDComponent>().ID;
        else
            out << YAML::Key << "Entity" << YAML::Value << 0;
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
                Entity parent{hc.Parent, entity.GetScene()};
                if (parent.HasComponent<IDComponent>())
                    parentUUID = (uint64_t)parent.GetComponent<IDComponent>().ID;
            }
            out << YAML::Key << "Parent" << YAML::Value << parentUUID;

            out << YAML::Key << "Children" << YAML::BeginSeq;
            for (auto childHandle : hc.Children)
            {
                Entity child{childHandle, entity.GetScene()};
                if (child.HasComponent<IDComponent>())
                    out << (uint64_t)child.GetComponent<IDComponent>().ID;
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
            outTask.parent = h["Parent"].as<uint64_t>();
            if (h["Children"])
            {
                for (auto child : h["Children"])
                    outTask.children.push_back(child.as<uint64_t>());
            }
        }
    }

    // --- UI Layout Helpers (Consolidated) ---

    static void serializeTextStyle(YAML::Emitter& out, const TextStyle& style) {
        out << YAML::BeginMap;
        out << YAML::Key << "FontName" << YAML::Value << style.FontName;
        out << YAML::Key << "FontSize" << YAML::Value << style.FontSize;
        out << YAML::Key << "TextColor" << YAML::Value << style.TextColor;
        out << YAML::Key << "Shadow" << YAML::Value << style.Shadow;
        out << YAML::Key << "ShadowOffset" << YAML::Value << style.ShadowOffset;
        out << YAML::Key << "ShadowColor" << YAML::Value << style.ShadowColor;
        out << YAML::Key << "LetterSpacing" << YAML::Value << style.LetterSpacing;
        out << YAML::Key << "LineHeight" << YAML::Value << style.LineHeight;
        out << YAML::Key << "HorizontalAlignment" << YAML::Value << (int)style.HorizontalAlignment;
        out << YAML::Key << "VerticalAlignment" << YAML::Value << (int)style.VerticalAlignment;
        out << YAML::EndMap;
    }

    static void deserializeTextStyle(TextStyle& style, YAML::Node node) {
        if (node["FontName"]) style.FontName = node["FontName"].as<std::string>();
        if (node["FontSize"]) style.FontSize = node["FontSize"].as<float>();
        if (node["TextColor"]) style.TextColor = node["TextColor"].as<Color>();
        if (node["Shadow"]) style.Shadow = node["Shadow"].as<bool>();
        if (node["ShadowOffset"]) style.ShadowOffset = node["ShadowOffset"].as<float>();
        if (node["ShadowColor"]) style.ShadowColor = node["ShadowColor"].as<Color>();
        if (node["LetterSpacing"]) style.LetterSpacing = node["LetterSpacing"].as<float>();
        if (node["LineHeight"]) style.LineHeight = node["LineHeight"].as<float>();
        if (node["HorizontalAlignment"]) style.HorizontalAlignment = (TextAlignment)node["HorizontalAlignment"].as<int>();
        if (node["VerticalAlignment"]) style.VerticalAlignment = (TextAlignment)node["VerticalAlignment"].as<int>();
    }

    static void serializeRectTransform(YAML::Emitter& out, const RectTransform& transform) {
        out << YAML::BeginMap;
        out << YAML::Key << "AnchorMin" << YAML::Value << transform.AnchorMin;
        out << YAML::Key << "AnchorMax" << YAML::Value << transform.AnchorMax;
        out << YAML::Key << "OffsetMin" << YAML::Value << transform.OffsetMin;
        out << YAML::Key << "OffsetMax" << YAML::Value << transform.OffsetMax;
        out << YAML::Key << "Pivot" << YAML::Value << transform.Pivot;
        out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
        out << YAML::Key << "Scale" << YAML::Value << transform.Scale;
        out << YAML::EndMap;
    }

    static void deserializeRectTransform(RectTransform& transform, YAML::Node node) {
        if (node["AnchorMin"]) transform.AnchorMin = node["AnchorMin"].as<Vector2>();
        if (node["AnchorMax"]) transform.AnchorMax = node["AnchorMax"].as<Vector2>();
        if (node["OffsetMin"]) transform.OffsetMin = node["OffsetMin"].as<Vector2>();
        if (node["OffsetMax"]) transform.OffsetMax = node["OffsetMax"].as<Vector2>();
        if (node["Pivot"]) transform.Pivot = node["Pivot"].as<Vector2>();
        if (node["Rotation"]) transform.Rotation = node["Rotation"].as<float>();
        if (node["Scale"]) transform.Scale = node["Scale"].as<Vector2>();
    }

    static void serializeUIStyle(YAML::Emitter& out, const UIStyle& style) {
        out << YAML::BeginMap;
        out << YAML::Key << "BackgroundColor" << YAML::Value << style.BackgroundColor;
        out << YAML::Key << "HoverColor" << YAML::Value << style.HoverColor;
        out << YAML::Key << "PressedColor" << YAML::Value << style.PressedColor;
        out << YAML::Key << "Rounding" << YAML::Value << style.Rounding;
        out << YAML::Key << "BorderSize" << YAML::Value << style.BorderSize;
        out << YAML::Key << "BorderColor" << YAML::Value << style.BorderColor;
        out << YAML::Key << "Padding" << YAML::Value << style.Padding;
        out << YAML::EndMap;
    }

    static void deserializeUIStyle(UIStyle& style, YAML::Node node) {
        if (node["BackgroundColor"]) style.BackgroundColor = node["BackgroundColor"].as<Color>();
        if (node["HoverColor"]) style.HoverColor = node["HoverColor"].as<Color>();
        if (node["PressedColor"]) style.PressedColor = node["PressedColor"].as<Color>();
        if (node["Rounding"]) style.Rounding = node["Rounding"].as<float>();
        if (node["BorderSize"]) style.BorderSize = node["BorderSize"].as<float>();
        if (node["BorderColor"]) style.BorderColor = node["BorderColor"].as<Color>();
        if (node["Padding"]) style.Padding = node["Padding"].as<float>();
    }

    // ========================================================================
    // Initialize Registry
    // ========================================================================

    void ComponentSerializer::Initialize()
    {
        s_Registry.clear();

        // --- Graphics ---
        Register<TransformComponent>("TransformComponent", [](auto& archive, auto& component) {
            archive.Property("Translation", component.Translation)
                   .Property("Rotation", component.Rotation)
                   .Property("Scale", component.Scale);
        });

        Register<ModelComponent>("ModelComponent", [](auto& archive, auto& component) {
            archive.Handle("ModelHandle", component.ModelHandle)
                   .Path("ModelPath", component.ModelPath);
        });

        Register<PointLightComponent>("PointLightComponent", [](auto& archive, auto& component) {
            archive.Property("LightColor", component.LightColor)
                   .Property("Intensity", component.Intensity)
                   .Property("Radius", component.Radius);
        });

        Register<SpotLightComponent>("SpotLightComponent", [](auto& archive, auto& component) {
            archive.Property("LightColor", component.LightColor)
                   .Property("Intensity", component.Intensity)
                   .Property("Range", component.Range)
                   .Property("InnerCutoff", component.InnerCutoff)
                   .Property("OuterCutoff", component.OuterCutoff);
        });

        Register<ShaderComponent>("ShaderComponent", [](auto& archive, auto& component) {
            archive.Path("ShaderPath", component.ShaderPath);
        });

        // --- Physics ---
        Register<ColliderComponent>("ColliderComponent", [](auto& archive, auto& component) {
            archive.Property("Type", (int&)component.Type)
                   .Property("Enabled", component.Enabled)
                   .Property("Offset", component.Offset)
                   .Property("Size", component.Size)
                   .Property("Radius", component.Radius)
                   .Property("Height", component.Height)
                   .Handle("ModelHandle", component.ModelHandle)
                   .Path("ModelPath", component.ModelPath)
                   .Property("AutoCalculate", component.AutoCalculate);
        });

        Register<RigidBodyComponent>("RigidBodyComponent", [](auto& archive, auto& component) {
            archive.Property("Mass", component.Mass)
                   .Property("UseGravity", component.UseGravity)
                   .Property("IsKinematic", component.IsKinematic);
        });

        // --- Audio ---
        Register<AudioComponent>("AudioComponent", [](auto& archive, auto& component) {
            archive.Handle("SoundHandle", component.SoundHandle)
                   .Path("SoundPath", component.SoundPath)
                   .Property("Loop", component.Loop)
                   .Property("PlayOnStart", component.PlayOnStart)
                   .Property("Volume", component.Volume)
                   .Property("Pitch", component.Pitch);
        });

        // --- Gameplay ---
        Register<PlayerComponent>("PlayerComponent", [](auto& archive, auto& component) {
            archive.Property("MovementSpeed", component.MovementSpeed)
                   .Property("LookSensitivity", component.LookSensitivity)
                   .Property("JumpForce", component.JumpForce);
        });

        Register<NavigationComponent>("NavigationComponent", [](auto& archive, auto& component) {
            archive.Property("IsDefaultFocus", component.IsDefaultFocus);
        });

        Register<SpawnComponent>("SpawnComponent", [](auto& archive, auto& component) {
            archive.Property("SpawnZoneSize", component.ZoneSize)
                   .Handle("SpawnTextureHandle", component.TextureHandle)
                   .Path("SpawnTexturePath", component.TexturePath)
                   .Property("RenderSpawnZoneInScene", component.RenderSpawnZoneInScene);
        });

        Register<CameraComponent>("CameraComponent", [](auto& archive, auto& component) {
            archive.Property("Primary", component.Primary)
                   .Property("FixedAspectRatio", component.FixedAspectRatio)
                   .Property("IsOrbitCamera", component.IsOrbitCamera)
                   .Property("OrbitDistance", component.OrbitDistance)
                   .Property("OrbitYaw", component.OrbitYaw)
                   .Property("OrbitPitch", component.OrbitPitch)
                   .Property("LookSensitivity", component.LookSensitivity)
                   .Property("TargetEntityTag", component.TargetEntityTag);
            
            // Camera Internal Settings
            auto& camera = component.Camera;
            int projType = (int)camera.GetProjectionType();
            archive.Property("ProjectionType", projType);
            camera.SetProjectionType((ProjectionType)projType);

            if (camera.GetProjectionType() == ProjectionType::Perspective) {
                float fov = camera.GetPerspectiveVerticalFOV();
                float nearClip = camera.GetPerspectiveNearClip();
                float farClip = camera.GetPerspectiveFarClip();
                archive.Property("PerspectiveFOV", fov)
                       .Property("PerspectiveNear", nearClip)
                       .Property("PerspectiveFar", farClip);
                camera.SetPerspectiveVerticalFOV(fov);
                camera.SetPerspectiveNearClip(nearClip);
                camera.SetPerspectiveFarClip(farClip);
            } else {
                float size = camera.GetOrthographicSize();
                float nearClip = camera.GetOrthographicNearClip();
                float farClip = camera.GetOrthographicFarClip();
                archive.Property("OrthoSize", size)
                       .Property("OrthoNear", nearClip)
                       .Property("OrthoFar", farClip);
                camera.SetOrthographicSize(size);
                camera.SetOrthographicNearClip(nearClip);
                camera.SetOrthographicFarClip(farClip);
            }
        });

        Register<SpriteComponent>("SpriteComponent", [](auto& archive, auto& component) {
            archive.Handle("TextureHandle", component.TextureHandle)
                   .Path("TexturePath", component.TexturePath)
                   .Property("Tint", component.Tint)
                   .Property("FlipX", component.FlipX)
                   .Property("FlipY", component.FlipY)
                   .Property("ZOrder", component.ZOrder);
        });

        // --- UI Components ---
        Register<ControlComponent>("ControlComponent", [](auto& archive, auto& component) {
            archive.Nested("Transform", component.Transform, serializeRectTransform, deserializeRectTransform);
            archive.Property("ZOrder", component.ZOrder)
                   .Property("IsActive", component.IsActive)
                   .Property("HiddenInHierarchy", component.HiddenInHierarchy);
        });

        Register<ButtonControl>("ButtonControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Interactable", component.IsInteractable);
            archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
            archive.Nested("Text", component.Text, serializeTextStyle, deserializeTextStyle);
        });

        Register<PanelControl>("PanelControl", [](auto& archive, auto& component) {
            archive.Handle("TextureHandle", component.TextureHandle)
                   .Path("TexturePath", component.TexturePath);
            archive.Nested("UIStyle", component.Style, serializeUIStyle, deserializeUIStyle);
        });

        Register<LabelControl>("LabelControl", [](auto& archive, auto& component) {
            archive.Property("Text", component.Text);
            archive.Nested("Style", component.Style, serializeTextStyle, deserializeTextStyle);
        });

        Register<SliderControl>("SliderControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Value", component.Value)
                   .Property("Min", component.Min)
                   .Property("Max", component.Max);
            archive.Nested("Text", component.Text, serializeTextStyle, deserializeTextStyle);
            archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
        });

        Register<CheckboxControl>("CheckboxControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Checked", component.Checked);
            archive.Nested("Text", component.Text, serializeTextStyle, deserializeTextStyle);
            archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
        });

        Register<ImageControl>("ImageControl", [](auto& archive, auto& component) {
            archive.Handle("TextureHandle", component.TextureHandle)
                   .Path("TexturePath", component.TexturePath);
            archive.Property("TintColor", component.TintColor)
                   .Property("BorderColor", component.BorderColor);
            archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
        });

        Register<ImageButtonControl>("ImageButtonControl", [](auto& archive, auto& component) {
            archive.Handle("TextureHandle", component.TextureHandle)
                   .Path("TexturePath", component.TexturePath)
                   .Property("Label", component.Label)
                   .Property("TintColor", component.TintColor)
                   .Property("BackgroundColor", component.BackgroundColor)
                   .Property("FramePadding", component.FramePadding);
            archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
        });

        Register<InputTextControl>("InputTextControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Text", component.Text)
                   .Property("Placeholder", component.Placeholder)
                   .Property("MaxLength", component.MaxLength)
                   .Property("Multiline", component.Multiline)
                   .Property("ReadOnly", component.ReadOnly)
                   .Property("Password", component.Password);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
            archive.Nested("BoxStyle", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
        });

        Register<ComboBoxControl>("ComboBoxControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Items", component.Items)
                   .Property("SelectedIndex", component.SelectedIndex);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
            archive.Nested("BoxStyle", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
        });

        Register<ProgressBarControl>("ProgressBarControl", [](auto& archive, auto& component) {
            archive.Property("Progress", component.Progress)
                   .Property("OverlayText", component.OverlayText)
                   .Property("ShowPercentage", component.ShowPercentage);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
            archive.Nested("BarStyle", component.BarStyle, serializeUIStyle, deserializeUIStyle);
        });

        Register<SeparatorControl>("SeparatorControl", [](auto& archive, auto& component) {
            archive.Property("Thickness", component.Thickness)
                   .Property("LineColor", component.LineColor);
        });

        Register<RadioButtonControl>("RadioButtonControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Options", component.Options)
                   .Property("SelectedIndex", component.SelectedIndex)
                   .Property("Horizontal", component.Horizontal);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
        });

        Register<ColorPickerControl>("ColorPickerControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("SelectedColor", component.SelectedColor)
                   .Property("ShowAlpha", component.ShowAlpha)
                   .Property("ShowPicker", component.ShowPicker);
            archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
        });

        Register<DragFloatControl>("DragFloatControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Value", component.Value)
                   .Property("Speed", component.Speed)
                   .Property("Min", component.Min)
                   .Property("Max", component.Max)
                   .Property("Format", component.Format);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
            archive.Nested("BoxStyle", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
        });

        Register<DragIntControl>("DragIntControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Value", component.Value)
                   .Property("Speed", component.Speed)
                   .Property("Min", component.Min)
                   .Property("Max", component.Max)
                   .Property("Format", component.Format);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
            archive.Nested("BoxStyle", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
        });

        Register<TreeNodeControl>("TreeNodeControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("IsOpen", component.IsOpen)
                   .Property("DefaultOpen", component.DefaultOpen)
                   .Property("IsLeaf", component.IsLeaf);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
        });

        Register<TabBarControl>("TabBarControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Reorderable", component.Reorderable)
                   .Property("AutoSelectNewTabs", component.AutoSelectNewTabs);
            archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
        });

        Register<TabItemControl>("TabItemControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("IsOpen", component.IsOpen)
                   .Property("Selected", component.Selected);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
        });

        Register<CollapsingHeaderControl>("CollapsingHeaderControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("IsOpen", component.IsOpen)
                   .Property("DefaultOpen", component.DefaultOpen);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
        });

        Register<PlotLinesControl>("PlotLinesControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Values", component.Values)
                   .Property("OverlayText", component.OverlayText)
                   .Property("ScaleMin", component.ScaleMin)
                   .Property("ScaleMax", component.ScaleMax)
                   .Property("GraphSize", component.GraphSize);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
            archive.Nested("BoxStyle", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
        });

        Register<PlotHistogramControl>("PlotHistogramControl", [](auto& archive, auto& component) {
            archive.Property("Label", component.Label)
                   .Property("Values", component.Values)
                   .Property("OverlayText", component.OverlayText)
                   .Property("ScaleMin", component.ScaleMin)
                   .Property("ScaleMax", component.ScaleMax)
                   .Property("GraphSize", component.GraphSize);
            archive.Nested("TextStyle", component.Style, serializeTextStyle, deserializeTextStyle);
            archive.Nested("BoxStyle", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
        });

        // --- Native Script Component (Manual due to registry access) ---
        {
            ComponentSerializerEntry entry;
            entry.YamlKey = "NativeScriptComponent";
            entry.Serialize = [](YAML::Emitter& out, Entity entity) {
                if (entity.HasComponent<NativeScriptComponent>()) {
                    out << YAML::Key << "NativeScriptComponent" << YAML::Value;
                    out << YAML::BeginMap;
                    auto& component = entity.GetComponent<NativeScriptComponent>();
                    out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
                    for (const auto& script : component.Scripts) out << script.ScriptName;
                    out << YAML::EndSeq;
                    out << YAML::EndMap;
                }
            };
            entry.Deserialize = [](Entity entity, YAML::Node node) {
                 if (node["NativeScriptComponent"]) {
                    auto componentNode = node["NativeScriptComponent"];
                    if (!entity.HasComponent<NativeScriptComponent>())
                        entity.AddComponent<NativeScriptComponent>();
                    
                    auto& component = entity.GetComponent<NativeScriptComponent>();
                    component.Scripts.clear();
                    if (entity.GetScene() && componentNode["Scripts"]) {
                        for (auto item : componentNode["Scripts"])
                            entity.GetScene()->GetScriptRegistry().AddScript(item.as<std::string>(), component);
                    }
                 }
            };
            entry.Copy = [](Entity source, Entity destination) {
                if (source.HasComponent<NativeScriptComponent>()) {
                     destination.AddOrReplaceComponent<NativeScriptComponent>(source.GetComponent<NativeScriptComponent>());
                }
            };
            s_Registry.push_back(entry);
        }
    }

    void ComponentSerializer::SerializeAll(YAML::Emitter& out, Entity entity)
    {
        for (auto& entry : s_Registry)
            entry.Serialize(out, entity);
        SerializeHierarchy(out, entity);
    }

    void ComponentSerializer::DeserializeAll(Entity entity, YAML::Node node)
    {
        for (const auto& entry : s_Registry)
            if (entry.Deserialize) entry.Deserialize(entity, node);
    }

    void ComponentSerializer::CopyAll(Entity source, Entity destination)
    {
        for (const auto& entry : s_Registry)
            if (entry.Copy) entry.Copy(source, destination);
    }

} // namespace CHEngine
