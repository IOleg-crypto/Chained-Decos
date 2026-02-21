#include "component_serializer.h"
#include "components.h"
#include "components/hierarchy_component.h"
#include "components/id_component.h"
#include "engine/core/yaml.h"
#include "engine/scene/serialization_utils.h"
#include "scene.h"
#include "script_registry.h"

namespace CHEngine
{
using namespace SerializationUtils;

std::vector<ComponentSerializerEntry> ComponentSerializer::s_Registry;

void ComponentSerializer::RegisterCustom(const ComponentSerializerEntry& entry)
{
    s_Registry.push_back(entry);
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
            Entity parent{hc.Parent, &entity.GetRegistry()};
            if (parent.HasComponent<IDComponent>())
            {
                parentUUID = (uint64_t)parent.GetComponent<IDComponent>().ID;
            }
        }
        out << YAML::Key << "Parent" << YAML::Value << parentUUID;

        out << YAML::Key << "Children" << YAML::BeginSeq;
        for (auto childHandle : hc.Children)
        {
            Entity child{childHandle, &entity.GetRegistry()};
            if (child.HasComponent<IDComponent>())
            {
                out << (uint64_t)child.GetComponent<IDComponent>().ID;
            }
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
            {
                outTask.children.push_back(child.as<uint64_t>());
            }
        }
    }
}

// --- UI Layout Helpers (Consolidated) ---

void ComponentSerializer::SerializeTextStyle(YAML::Emitter& out, const TextStyle& style)
{
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

void ComponentSerializer::DeserializeTextStyle(TextStyle& style, YAML::Node node)
{
    if (node["FontName"])
    {
        style.FontName = node["FontName"].as<std::string>();
    }
    if (node["FontSize"])
    {
        style.FontSize = node["FontSize"].as<float>();
    }
    if (node["TextColor"])
    {
        style.TextColor = node["TextColor"].as<Color>();
    }
    if (node["Shadow"])
    {
        style.Shadow = node["Shadow"].as<bool>();
    }
    if (node["ShadowOffset"])
    {
        style.ShadowOffset = node["ShadowOffset"].as<float>();
    }
    if (node["ShadowColor"])
    {
        style.ShadowColor = node["ShadowColor"].as<Color>();
    }
    if (node["LetterSpacing"])
    {
        style.LetterSpacing = node["LetterSpacing"].as<float>();
    }
    if (node["LineHeight"])
    {
        style.LineHeight = node["LineHeight"].as<float>();
    }
    if (node["HorizontalAlignment"])
    {
        style.HorizontalAlignment = (TextAlignment)node["HorizontalAlignment"].as<int>();
    }
    if (node["VerticalAlignment"])
    {
        style.VerticalAlignment = (TextAlignment)node["VerticalAlignment"].as<int>();
    }
}

void ComponentSerializer::SerializeRectTransform(YAML::Emitter& out, const RectTransform& transform)
{
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

void ComponentSerializer::DeserializeRectTransform(RectTransform& transform, YAML::Node node)
{
    if (node["AnchorMin"])
    {
        transform.AnchorMin = node["AnchorMin"].as<Vector2>();
    }
    if (node["AnchorMax"])
    {
        transform.AnchorMax = node["AnchorMax"].as<Vector2>();
    }
    if (node["OffsetMin"])
    {
        transform.OffsetMin = node["OffsetMin"].as<Vector2>();
    }
    if (node["OffsetMax"])
    {
        transform.OffsetMax = node["OffsetMax"].as<Vector2>();
    }
    if (node["Pivot"])
    {
        transform.Pivot = node["Pivot"].as<Vector2>();
    }
    if (node["Rotation"])
    {
        transform.Rotation = node["Rotation"].as<float>();
    }
    if (node["Scale"])
    {
        transform.Scale = node["Scale"].as<Vector2>();
    }
}

void ComponentSerializer::SerializeUIStyle(YAML::Emitter& out, const UIStyle& style)
{
    out << YAML::BeginMap;
    out << YAML::Key << "BackgroundColor" << YAML::Value << style.BackgroundColor;
    out << YAML::Key << "HoverColor" << YAML::Value << style.HoverColor;
    out << YAML::Key << "PressedColor" << YAML::Value << style.PressedColor;
    out << YAML::Key << "Rounding" << YAML::Value << style.Rounding;
    out << YAML::Key << "BorderSize" << YAML::Value << style.BorderSize;
    out << YAML::Key << "BorderColor" << YAML::Value << style.BorderColor;
    out << YAML::Key << "Padding" << YAML::Value << style.Padding;
    out << YAML::Key << "UseGradient" << YAML::Value << style.UseGradient;
    out << YAML::Key << "GradientColor" << YAML::Value << style.GradientColor;
    out << YAML::Key << "HoverScale" << YAML::Value << style.HoverScale;
    out << YAML::Key << "PressedScale" << YAML::Value << style.PressedScale;
    out << YAML::Key << "TransitionSpeed" << YAML::Value << style.TransitionSpeed;
    out << YAML::EndMap;
}

void ComponentSerializer::DeserializeUIStyle(UIStyle& style, YAML::Node node)
{
    if (node["BackgroundColor"])
    {
        style.BackgroundColor = node["BackgroundColor"].as<Color>();
    }
    if (node["HoverColor"])
    {
        style.HoverColor = node["HoverColor"].as<Color>();
    }
    if (node["PressedColor"])
    {
        style.PressedColor = node["PressedColor"].as<Color>();
    }
    if (node["Rounding"])
    {
        style.Rounding = node["Rounding"].as<float>();
    }
    if (node["BorderSize"])
    {
        style.BorderSize = node["BorderSize"].as<float>();
    }
    if (node["BorderColor"])
    {
        style.BorderColor = node["BorderColor"].as<Color>();
    }
    if (node["Padding"])
    {
        style.Padding = node["Padding"].as<float>();
    }
    if (node["UseGradient"])
    {
        style.UseGradient = node["UseGradient"].as<bool>();
    }
    if (node["GradientColor"])
    {
        style.GradientColor = node["GradientColor"].as<Color>();
    }
    if (node["HoverScale"])
    {
        style.HoverScale = node["HoverScale"].as<float>();
    }
    if (node["PressedScale"])
    {
        style.PressedScale = node["PressedScale"].as<float>();
    }
    if (node["TransitionSpeed"])
    {
        style.TransitionSpeed = node["TransitionSpeed"].as<float>();
    }
}

void ComponentSerializer::SerializeMaterialInstance(YAML::Emitter& out, const MaterialInstance& mat)
{
    out << YAML::BeginMap;
    out << YAML::Key << "AlbedoColor" << YAML::Value << mat.AlbedoColor;
    out << YAML::Key << "AlbedoPath" << YAML::Value << Project::GetRelativePath(mat.AlbedoPath);
    out << YAML::Key << "OverrideAlbedo" << YAML::Value << mat.OverrideAlbedo;

    out << YAML::Key << "NormalMapPath" << YAML::Value << Project::GetRelativePath(mat.NormalMapPath);
    out << YAML::Key << "OverrideNormal" << YAML::Value << mat.OverrideNormal;

    out << YAML::Key << "MetallicRoughnessPath" << YAML::Value << Project::GetRelativePath(mat.MetallicRoughnessPath);
    out << YAML::Key << "OverrideMetallicRoughness" << YAML::Value << mat.OverrideMetallicRoughness;

    out << YAML::Key << "OcclusionMapPath" << YAML::Value << Project::GetRelativePath(mat.OcclusionMapPath);
    out << YAML::Key << "OverrideOcclusion" << YAML::Value << mat.OverrideOcclusion;

    out << YAML::Key << "EmissivePath" << YAML::Value << Project::GetRelativePath(mat.EmissivePath);
    out << YAML::Key << "EmissiveColor" << YAML::Value << mat.EmissiveColor;
    out << YAML::Key << "EmissiveIntensity" << YAML::Value << mat.EmissiveIntensity;
    out << YAML::Key << "OverrideEmissive" << YAML::Value << mat.OverrideEmissive;

    out << YAML::Key << "Metalness" << YAML::Value << mat.Metalness;
    out << YAML::Key << "Roughness" << YAML::Value << mat.Roughness;

    out << YAML::Key << "DoubleSided" << YAML::Value << mat.DoubleSided;
    out << YAML::Key << "Transparent" << YAML::Value << mat.Transparent;
    out << YAML::Key << "Alpha" << YAML::Value << mat.Alpha;
    out << YAML::EndMap;
}

void ComponentSerializer::DeserializeMaterialInstance(MaterialInstance& mat, YAML::Node node)
{
    if (node["AlbedoColor"])
    {
        mat.AlbedoColor = node["AlbedoColor"].as<Color>();
    }
    if (node["AlbedoPath"])
    {
        mat.AlbedoPath = node["AlbedoPath"].as<std::string>();
    }
    if (node["OverrideAlbedo"])
    {
        mat.OverrideAlbedo = node["OverrideAlbedo"].as<bool>();
    }

    if (node["NormalMapPath"])
    {
        mat.NormalMapPath = node["NormalMapPath"].as<std::string>();
    }
    if (node["OverrideNormal"])
    {
        mat.OverrideNormal = node["OverrideNormal"].as<bool>();
    }

    if (node["MetallicRoughnessPath"])
    {
        mat.MetallicRoughnessPath = node["MetallicRoughnessPath"].as<std::string>();
    }
    if (node["OverrideMetallicRoughness"])
    {
        mat.OverrideMetallicRoughness = node["OverrideMetallicRoughness"].as<bool>();
    }

    if (node["OcclusionMapPath"])
    {
        mat.OcclusionMapPath = node["OcclusionMapPath"].as<std::string>();
    }
    if (node["OverrideOcclusion"])
    {
        mat.OverrideOcclusion = node["OverrideOcclusion"].as<bool>();
    }

    if (node["EmissivePath"])
    {
        mat.EmissivePath = node["EmissivePath"].as<std::string>();
    }
    if (node["EmissiveColor"])
    {
        mat.EmissiveColor = node["EmissiveColor"].as<Color>();
    }
    if (node["EmissiveIntensity"])
    {
        mat.EmissiveIntensity = node["EmissiveIntensity"].as<float>();
    }
    if (node["OverrideEmissive"])
    {
        mat.OverrideEmissive = node["OverrideEmissive"].as<bool>();
    }

    if (node["Metalness"])
    {
        mat.Metalness = node["Metalness"].as<float>();
    }
    if (node["Roughness"])
    {
        mat.Roughness = node["Roughness"].as<float>();
    }

    if (node["DoubleSided"])
    {
        mat.DoubleSided = node["DoubleSided"].as<bool>();
    }
    if (node["Transparent"])
    {
        mat.Transparent = node["Transparent"].as<bool>();
    }
    if (node["Alpha"])
    {
        mat.Alpha = node["Alpha"].as<float>();
    }
}

void ComponentSerializer::SerializeMaterialSlot(YAML::Emitter& out, const MaterialSlot& slot)
{
    out << YAML::BeginMap;
    out << YAML::Key << "Name" << YAML::Value << slot.Name;
    out << YAML::Key << "Index" << YAML::Value << slot.Index;
    out << YAML::Key << "Target" << YAML::Value << (int)slot.Target;
    out << YAML::Key << "Material" << YAML::Value;
    SerializeMaterialInstance(out, slot.Material);
    out << YAML::EndMap;
}

void ComponentSerializer::DeserializeMaterialSlot(MaterialSlot& slot, YAML::Node node)
{
    if (node["Name"])
    {
        slot.Name = node["Name"].as<std::string>();
    }
    if (node["Index"])
    {
        slot.Index = node["Index"].as<int>();
    }
    if (node["Target"])
    {
        slot.Target = (MaterialSlotTarget)node["Target"].as<int>();
    }
    if (node["Material"])
    {
        DeserializeMaterialInstance(slot.Material, node["Material"]);
    }
}

// ========================================================================
// Initialize Registry
// ========================================================================

void ComponentSerializer::Initialize()
{
    s_Registry.clear();

    // --- Основні ---
    Register<TagComponent>("TagComponent",
                           [](auto& archive, auto& component) { archive.Property("Tag", component.Tag); });

    // --- Graphics ---
    Register<TransformComponent>("TransformComponent", [](auto& archive, auto& component) {
        archive.Property("Translation", component.Translation)
            .Property("Rotation", component.Rotation)
            .Property("Scale", component.Scale);

        // Recalculate Quat from Euler after deserialization
        if (archive.GetMode() == SerializationUtils::PropertyArchive::Deserialize)
        {
            component.SetRotation(component.Rotation);
        }
    });

    Register<ModelComponent>("ModelComponent", [](auto& archive, auto& component) {
        archive.Handle("ModelHandle", component.ModelHandle).Path("ModelPath", component.ModelPath);

        if (archive.GetMode() == SerializationUtils::PropertyArchive::Serialize)
        {
            auto& out = *archive.GetEmitter();
            out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
            for (const auto& slot : component.Materials)
            {
                SerializeMaterialSlot(out, slot);
            }
            out << YAML::EndSeq;
        }
        else
        {
            auto node = archive.GetNode();
            if (node["Materials"] && node["Materials"].IsSequence())
            {
                component.Materials.clear();
                for (auto slotNode : node["Materials"])
                {
                    MaterialSlot slot;
                    DeserializeMaterialSlot(slot, slotNode);
                    component.Materials.push_back(slot);
                }
                component.MaterialsInitialized = true;
            }
        }
    });

    Register<LightComponent>("LightComponent", [](auto& archive, auto& component) {
        archive.Property("Type", (int&)component.Type)
            .Property("LightColor", component.LightColor)
            .Property("Intensity", component.Intensity)
            .Property("Radius", component.Radius)
            .Property("InnerCutoff", component.InnerCutoff)
            .Property("OuterCutoff", component.OuterCutoff);
    });

    Register<ShaderComponent>("ShaderComponent",
                              [](auto& archive, auto& component) { archive.Path("ShaderPath", component.ShaderPath); });

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

    Register<SceneTransitionComponent>("SceneTransitionComponent", [](auto& archive, auto& component) {
        archive.Property("TargetScenePath", component.TargetScenePath).Property("Triggered", component.Triggered);
    });

    Register<AnimationComponent>("AnimationComponent", [](auto& archive, auto& component) {
        archive.Property("AnimationPath", component.AnimationPath)
            .Property("CurrentAnimationIndex", component.CurrentAnimationIndex)
            .Property("IsLooping", component.IsLooping)
            .Property("IsPlaying", component.IsPlaying);
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

        if (camera.GetProjectionType() == ProjectionType::Perspective)
        {
            float fov = camera.GetPerspectiveVerticalFOV();
            float nearClip = camera.GetPerspectiveNearClip();
            float farClip = camera.GetPerspectiveFarClip();
            archive.Property("PerspectiveFOV", fov)
                .Property("PerspectiveNear", nearClip)
                .Property("PerspectiveFar", farClip);
            camera.SetPerspectiveVerticalFOV(fov);
            camera.SetPerspectiveNearClip(nearClip);
            camera.SetPerspectiveFarClip(farClip);
        }
        else
        {
            float size = camera.GetOrthographicSize();
            float nearClip = camera.GetOrthographicNearClip();
            float farClip = camera.GetOrthographicFarClip();
            archive.Property("OrthoSize", size).Property("OrthoNear", nearClip).Property("OrthoFar", farClip);
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
        archive.Nested("Transform", component.Transform, SerializeRectTransform, DeserializeRectTransform);
        archive.Property("ZOrder", component.ZOrder)
            .Property("IsActive", component.IsActive)
            .Property("HiddenInHierarchy", component.HiddenInHierarchy);
    });

    Register<ButtonControl>("ButtonControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("Interactable", component.IsInteractable)
            .Property("AutoSize", component.AutoSize);
        archive.Nested("Style", component.Style, SerializeUIStyle, DeserializeUIStyle);
        archive.Nested("Text", component.Text, SerializeTextStyle, DeserializeTextStyle);
    });

    Register<PanelControl>("PanelControl", [](auto& archive, auto& component) {
        archive.Handle("TextureHandle", component.TextureHandle)
            .Path("TexturePath", component.TexturePath)
            .Property("FullScreen", component.FullScreen);
        archive.Nested("UIStyle", component.Style, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<LabelControl>("LabelControl", [](auto& archive, auto& component) {
        archive.Property("Text", component.Text).Property("AutoSize", component.AutoSize);
        archive.Nested("Style", component.Style, SerializeTextStyle, DeserializeTextStyle);
    });

    Register<SliderControl>("SliderControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("Value", component.Value)
            .Property("Min", component.Min)
            .Property("Max", component.Max);
        archive.Nested("Text", component.Text, SerializeTextStyle, DeserializeTextStyle);
        archive.Nested("Style", component.Style, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<CheckboxControl>("CheckboxControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label).Property("Checked", component.Checked);
        archive.Nested("Text", component.Text, SerializeTextStyle, DeserializeTextStyle);
        archive.Nested("Style", component.Style, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<ImageControl>("ImageControl", [](auto& archive, auto& component) {
        archive.Handle("TextureHandle", component.TextureHandle).Path("TexturePath", component.TexturePath);
        archive.Property("TintColor", component.TintColor).Property("BorderColor", component.BorderColor);
        archive.Nested("Style", component.Style, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<ImageButtonControl>("ImageButtonControl", [](auto& archive, auto& component) {
        archive.Handle("TextureHandle", component.TextureHandle)
            .Path("TexturePath", component.TexturePath)
            .Property("Label", component.Label)
            .Property("TintColor", component.TintColor)
            .Property("BackgroundColor", component.BackgroundColor)
            .Property("FramePadding", component.FramePadding);
        archive.Nested("Style", component.Style, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<InputTextControl>("InputTextControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("Text", component.Text)
            .Property("Placeholder", component.Placeholder)
            .Property("MaxLength", component.MaxLength)
            .Property("Multiline", component.Multiline)
            .Property("ReadOnly", component.ReadOnly)
            .Property("Password", component.Password);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
        archive.Nested("BoxStyle", component.BoxStyle, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<ComboBoxControl>("ComboBoxControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("Items", component.Items)
            .Property("SelectedIndex", component.SelectedIndex);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
        archive.Nested("BoxStyle", component.BoxStyle, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<ProgressBarControl>("ProgressBarControl", [](auto& archive, auto& component) {
        archive.Property("Progress", component.Progress)
            .Property("OverlayText", component.OverlayText)
            .Property("ShowPercentage", component.ShowPercentage);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
        archive.Nested("BarStyle", component.BarStyle, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<SeparatorControl>("SeparatorControl", [](auto& archive, auto& component) {
        archive.Property("Thickness", component.Thickness).Property("LineColor", component.LineColor);
    });

    Register<RadioButtonControl>("RadioButtonControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("Options", component.Options)
            .Property("SelectedIndex", component.SelectedIndex)
            .Property("Horizontal", component.Horizontal);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
    });

    Register<ColorPickerControl>("ColorPickerControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("SelectedColor", component.SelectedColor)
            .Property("ShowAlpha", component.ShowAlpha)
            .Property("ShowPicker", component.ShowPicker);
        archive.Nested("Style", component.Style, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<DragFloatControl>("DragFloatControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("Value", component.Value)
            .Property("Speed", component.Speed)
            .Property("Min", component.Min)
            .Property("Max", component.Max)
            .Property("Format", component.Format);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
        archive.Nested("BoxStyle", component.BoxStyle, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<DragIntControl>("DragIntControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("Value", component.Value)
            .Property("Speed", component.Speed)
            .Property("Min", component.Min)
            .Property("Max", component.Max)
            .Property("Format", component.Format);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
        archive.Nested("BoxStyle", component.BoxStyle, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<TreeNodeControl>("TreeNodeControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("IsOpen", component.IsOpen)
            .Property("DefaultOpen", component.DefaultOpen)
            .Property("IsLeaf", component.IsLeaf);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
    });

    Register<TabBarControl>("TabBarControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("Reorderable", component.Reorderable)
            .Property("AutoSelectNewTabs", component.AutoSelectNewTabs);
        archive.Nested("Style", component.Style, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<TabItemControl>("TabItemControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("IsOpen", component.IsOpen)
            .Property("Selected", component.Selected);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
    });

    Register<CollapsingHeaderControl>("CollapsingHeaderControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("IsOpen", component.IsOpen)
            .Property("DefaultOpen", component.DefaultOpen);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
    });

    Register<PlotLinesControl>("PlotLinesControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("Values", component.Values)
            .Property("OverlayText", component.OverlayText)
            .Property("ScaleMin", component.ScaleMin)
            .Property("ScaleMax", component.ScaleMax)
            .Property("GraphSize", component.GraphSize);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
        archive.Nested("BoxStyle", component.BoxStyle, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<PlotHistogramControl>("PlotHistogramControl", [](auto& archive, auto& component) {
        archive.Property("Label", component.Label)
            .Property("Values", component.Values)
            .Property("OverlayText", component.OverlayText)
            .Property("ScaleMin", component.ScaleMin)
            .Property("ScaleMax", component.ScaleMax)
            .Property("GraphSize", component.GraphSize);
        archive.Nested("TextStyle", component.Style, SerializeTextStyle, DeserializeTextStyle);
        archive.Nested("BoxStyle", component.BoxStyle, SerializeUIStyle, DeserializeUIStyle);
    });

    Register<VerticalLayoutGroup>("VerticalLayoutGroup", [](auto& archive, auto& component) {
        archive.Property("Spacing", component.Spacing).Property("Padding", component.Padding);
    });

    // --- Native Script Component ---
    {
        ComponentSerializerEntry entry;
        entry.Key = "NativeScriptComponent";
        entry.Serialize = [](YAML::Emitter& out, Entity entity) {
            if (entity.HasComponent<NativeScriptComponent>())
            {
                out << YAML::Key << "NativeScriptComponent" << YAML::Value;
                out << YAML::BeginMap;
                auto& component = entity.GetComponent<NativeScriptComponent>();
                out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
                for (const auto& script : component.Scripts)
                {
                    out << script.ScriptName;
                }
                out << YAML::EndSeq;
                out << YAML::EndMap;
            }
        };
        entry.Deserialize = [](Entity entity, YAML::Node node) {
            if (node["NativeScriptComponent"])
            {
                auto componentNode = node["NativeScriptComponent"];
                if (!entity.HasComponent<NativeScriptComponent>())
                {
                    entity.AddComponent<NativeScriptComponent>();
                }

                auto& component = entity.GetComponent<NativeScriptComponent>();
                component.Scripts.clear();
                auto* scene = entity.GetRegistry().ctx().get<Scene*>();
                if (scene && componentNode["Scripts"])
                {
                    for (auto item : componentNode["Scripts"])
                    {
                        std::string scriptName = item.as<std::string>();
                        CH_CORE_INFO("ComponentSerializer: Adding script '{}' to entity '{}'", scriptName,
                                     entity.GetName());
                        scene->GetScriptRegistry().AddScript(scriptName, component);
                    }
                }
                else if (!scene)
                {
                    CH_CORE_WARN("ComponentSerializer: Could not find Scene in registry context!");
                }
            }
        };
        entry.Copy = [](Entity source, Entity destination) {
            if (source.HasComponent<NativeScriptComponent>())
            {
                destination.AddOrReplaceComponent<NativeScriptComponent>(source.GetComponent<NativeScriptComponent>());
            }
        };
        RegisterCustom(entry);
    }
}

void ComponentSerializer::SerializeAll(YAML::Emitter& out, Entity entity)
{
    for (auto& entry : s_Registry)
    {
        entry.Serialize(out, entity);
    }
    SerializeHierarchy(out, entity);
}

void ComponentSerializer::DeserializeAll(Entity entity, YAML::Node node)
{
    for (const auto& entry : s_Registry)
    {
        if (entry.Deserialize)
        {
            entry.Deserialize(entity, node);
        }
    }
}

void ComponentSerializer::CopyAll(Entity source, Entity destination)
{
    for (const auto& entry : s_Registry)
    {
        if (entry.Copy)
        {
            entry.Copy(source, destination);
        }
    }
}

} // namespace CHEngine
