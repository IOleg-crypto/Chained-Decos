#include "component_serializer.h"
#include "scene.h"
#include "components.h"
#include "engine/core/yaml.h"
#include "script_registry.h"

namespace CHEngine
{
    // === Serialization ===

    void ComponentSerializer::SerializeID(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<IDComponent>())
            out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entity.GetComponent<IDComponent>().ID;
        else
            out << YAML::Key << "Entity" << YAML::Value << 0;
    }

    void ComponentSerializer::SerializeTag(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            out << YAML::Key << "TagComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "Tag" << YAML::Value << entity.GetComponent<TagComponent>().Tag;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeTransform(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<TransformComponent>())
        {
            out << YAML::Key << "TransformComponent";
            auto &tc = entity.GetComponent<TransformComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
            out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
            out << YAML::Key << "Scale" << YAML::Value << tc.Scale;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeModel(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<ModelComponent>())
        {
            out << YAML::Key << "ModelComponent";
            auto &mc = entity.GetComponent<ModelComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "ModelPath" << YAML::Value << mc.ModelPath;

            if (!mc.Materials.empty())
            {
                out << YAML::Key << "Materials";
                out << YAML::BeginSeq;
                for (const auto &slot : mc.Materials)
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "Name" << YAML::Value << slot.Name;
                    out << YAML::Key << "Index" << YAML::Value << slot.Index;
                    out << YAML::Key << "Target" << YAML::Value << (int)slot.Target;
                    out << YAML::Key << "Material";
                    out << YAML::BeginMap;
                    out << YAML::Key << "AlbedoColor" << YAML::Value << slot.Material.AlbedoColor;
                    out << YAML::Key << "AlbedoPath" << YAML::Value << slot.Material.AlbedoPath;
                    out << YAML::Key << "OverrideAlbedo" << YAML::Value << slot.Material.OverrideAlbedo;
                    out << YAML::Key << "NormalMapPath" << YAML::Value << slot.Material.NormalMapPath;
                    out << YAML::Key << "OverrideNormal" << YAML::Value << slot.Material.OverrideNormal;
                    out << YAML::Key << "MetallicRoughnessPath" << YAML::Value << slot.Material.MetallicRoughnessPath;
                    out << YAML::Key << "OverrideMetallicRoughness" << YAML::Value
                        << slot.Material.OverrideMetallicRoughness;
                    out << YAML::Key << "EmissivePath" << YAML::Value << slot.Material.EmissivePath;
                    out << YAML::Key << "OverrideEmissive" << YAML::Value << slot.Material.OverrideEmissive;
                    out << YAML::Key << "Roughness" << YAML::Value << slot.Material.Roughness;
                    out << YAML::Key << "ShaderPath" << YAML::Value << slot.Material.ShaderPath;
                    out << YAML::Key << "OverrideShader" << YAML::Value << slot.Material.OverrideShader;
                    out << YAML::EndMap;
                    out << YAML::EndMap;
                }
                out << YAML::EndSeq;
            }
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeSpawn(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<SpawnComponent>())
        {
            out << YAML::Key << "SpawnComponent";
            auto &sc = entity.GetComponent<SpawnComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "IsActive" << YAML::Value << sc.IsActive;
            out << YAML::Key << "ZoneSize" << YAML::Value << sc.ZoneSize;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeCollider(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<ColliderComponent>())
        {
            out << YAML::Key << "ColliderComponent";
            auto &cc = entity.GetComponent<ColliderComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "Type" << YAML::Value << (int)cc.Type;
            out << YAML::Key << "Enabled" << YAML::Value << cc.Enabled;
            out << YAML::Key << "Offset" << YAML::Value << cc.Offset;
            out << YAML::Key << "Size" << YAML::Value << cc.Size;
            out << YAML::Key << "AutoCalculate" << YAML::Value << cc.AutoCalculate;
            out << YAML::Key << "ModelPath" << YAML::Value << cc.ModelPath;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializePointLight(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<PointLightComponent>())
        {
            out << YAML::Key << "PointLightComponent";
            auto &lc = entity.GetComponent<PointLightComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "LightColor" << YAML::Value << lc.LightColor;
            out << YAML::Key << "Intensity" << YAML::Value << lc.Intensity;
            out << YAML::Key << "Radius" << YAML::Value << lc.Radius;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeSpotLight(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<SpotLightComponent>())
        {
            out << YAML::Key << "SpotLightComponent";
            auto &lc = entity.GetComponent<SpotLightComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "LightColor" << YAML::Value << lc.LightColor;
            out << YAML::Key << "Intensity" << YAML::Value << lc.Intensity;
            out << YAML::Key << "Range" << YAML::Value << lc.Range;
            out << YAML::Key << "InnerCutoff" << YAML::Value << lc.InnerCutoff;
            out << YAML::Key << "OuterCutoff" << YAML::Value << lc.OuterCutoff;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeRigidBody(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<RigidBodyComponent>())
        {
            out << YAML::Key << "RigidBodyComponent";
            auto &rb = entity.GetComponent<RigidBodyComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "Velocity" << YAML::Value << rb.Velocity;
            out << YAML::Key << "UseGravity" << YAML::Value << rb.UseGravity;
            out << YAML::Key << "IsKinematic" << YAML::Value << rb.IsKinematic;
            out << YAML::Key << "Mass" << YAML::Value << rb.Mass;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializePlayer(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<PlayerComponent>())
        {
            out << YAML::Key << "PlayerComponent";
            out << YAML::BeginMap;
            auto &pc = entity.GetComponent<PlayerComponent>();
            out << YAML::Key << "MovementSpeed" << YAML::Value << pc.MovementSpeed;
            out << YAML::Key << "LookSensitivity" << YAML::Value << pc.LookSensitivity;
            out << YAML::Key << "CameraYaw" << YAML::Value << pc.CameraYaw;
            out << YAML::Key << "CameraPitch" << YAML::Value << pc.CameraPitch;
            out << YAML::Key << "CameraDistance" << YAML::Value << pc.CameraDistance;
            out << YAML::Key << "JumpForce" << YAML::Value << pc.JumpForce;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeSceneTransition(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<SceneTransitionComponent>())
        {
            out << YAML::Key << "SceneTransitionComponent";
            auto &stc = entity.GetComponent<SceneTransitionComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "TargetScenePath" << YAML::Value << stc.TargetScenePath;
            out << YAML::Key << "Triggered" << YAML::Value << stc.Triggered;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeBillboard(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<BillboardComponent>())
        {
            out << YAML::Key << "BillboardComponent";
            auto &bc = entity.GetComponent<BillboardComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "TexturePath" << YAML::Value << bc.TexturePath;
            out << YAML::Key << "Tint" << YAML::Value << bc.Tint;
            out << YAML::Key << "Size" << YAML::Value << bc.Size;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeNavigation(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<NavigationComponent>())
        {
            out << YAML::Key << "NavigationComponent";
            auto &nc = entity.GetComponent<NavigationComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "IsDefaultFocus" << YAML::Value << nc.IsDefaultFocus;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeShader(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<ShaderComponent>())
        {
            out << YAML::Key << "ShaderComponent";
            auto &sc = entity.GetComponent<ShaderComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "ShaderPath" << YAML::Value << sc.ShaderPath;
            out << YAML::Key << "Enabled" << YAML::Value << sc.Enabled;
            out << YAML::Key << "Uniforms" << YAML::Value << YAML::BeginSeq;
            for (const auto &u : sc.Uniforms)
            {
                out << YAML::BeginMap;
                out << YAML::Key << "" << YAML::Value << u.Name;
                out << YAML::Key << "Type" << YAML::Value << u.Type;
                out << YAML::Key << "Value" << YAML::Value
                    << std::vector<float>{u.Value[0], u.Value[1], u.Value[2], u.Value[3]};
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeAnimation(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<AnimationComponent>())
        {
            out << YAML::Key << "AnimationComponent";
            out << YAML::BeginMap;
            auto &animation = entity.GetComponent<AnimationComponent>();
            out << YAML::Key << "AnimationPath" << YAML::Value << animation.AnimationPath;
            out << YAML::Key << "CurrentAnimationIndex" << YAML::Value << animation.CurrentAnimationIndex;
            out << YAML::Key << "IsLooping" << YAML::Value << animation.IsLooping;
            out << YAML::Key << "IsPlaying" << YAML::Value << animation.IsPlaying;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeHierarchy(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<HierarchyComponent>())
        {
            out << YAML::Key << "HierarchyComponent";
            auto &hc = entity.GetComponent<HierarchyComponent>();
            out << YAML::BeginMap;
            if (hc.Parent != entt::null)
            {
                Entity parent{hc.Parent, entity.GetScene()};
                out << YAML::Key << "Parent" << YAML::Value << (uint64_t)parent.GetComponent<IDComponent>().ID;
            }
            else
            {
                out << YAML::Key << "Parent" << YAML::Value << 0;
            }

            out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
            for (auto childID : hc.Children)
            {
                Entity child{childID, entity.GetScene()};
                out << (uint64_t)child.GetComponent<IDComponent>().ID;
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeAudio(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<AudioComponent>())
        {
            out << YAML::Key << "AudioComponent";
            auto &ac = entity.GetComponent<AudioComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "SoundPath" << YAML::Value << ac.SoundPath;
            out << YAML::Key << "Volume" << YAML::Value << ac.Volume;
            out << YAML::Key << "Pitch" << YAML::Value << ac.Pitch;
            out << YAML::Key << "Loop" << YAML::Value << ac.Loop;
            out << YAML::Key << "PlayOnStart" << YAML::Value << ac.PlayOnStart;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeCamera(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<CameraComponent>())
        {
            out << YAML::Key << "CameraComponent";
            auto &cc = entity.GetComponent<CameraComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "Fov" << YAML::Value << cc.Fov;
            out << YAML::Key << "Offset" << YAML::Value << cc.Offset;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeNativeScript(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<NativeScriptComponent>())
        {
            out << YAML::Key << "NativeScriptComponent";
            auto &nsc = entity.GetComponent<NativeScriptComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
            for (const auto &script : nsc.Scripts)
            {
                out << YAML::BeginMap;
                out << YAML::Key << "ScriptName" << YAML::Value << script.ScriptName;
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::SerializeUI(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<ControlComponent>())
        {
            out << YAML::Key << "ControlComponent";
            auto &cc = entity.GetComponent<ControlComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "IsActive" << YAML::Value << cc.IsActive;
            out << YAML::Key << "HiddenInHierarchy" << YAML::Value << cc.HiddenInHierarchy;
            out << YAML::Key << "ZOrder" << YAML::Value << cc.ZOrder;

            out << YAML::Key << "RectTransform";
            out << YAML::BeginMap;
            out << YAML::Key << "AnchorMin" << YAML::Value << cc.Transform.AnchorMin;
            out << YAML::Key << "AnchorMax" << YAML::Value << cc.Transform.AnchorMax;
            out << YAML::Key << "OffsetMin" << YAML::Value << cc.Transform.OffsetMin;
            out << YAML::Key << "OffsetMax" << YAML::Value << cc.Transform.OffsetMax;
            out << YAML::Key << "Pivot" << YAML::Value << cc.Transform.Pivot;
            out << YAML::Key << "Rotation" << YAML::Value << cc.Transform.Rotation;
            out << YAML::Key << "Scale" << YAML::Value << cc.Transform.Scale;
            out << YAML::EndMap;

            out << YAML::EndMap;
        }

        if (entity.HasComponent<LabelControl>())
        {
            out << YAML::Key << "LabelControl";
            auto &lbl = entity.GetComponent<LabelControl>();
            out << YAML::BeginMap;
            out << YAML::Key << "Text" << YAML::Value << lbl.Text;
            
            out << YAML::Key << "Style" << YAML::BeginMap;
            out << YAML::Key << "FontName" << YAML::Value << lbl.Style.FontName;
            out << YAML::Key << "FontSize" << YAML::Value << lbl.Style.FontSize;
            out << YAML::Key << "TextColor" << YAML::Value << lbl.Style.TextColor;
            out << YAML::Key << "Shadow" << YAML::Value << lbl.Style.Shadow;
            out << YAML::Key << "ShadowOffset" << YAML::Value << lbl.Style.ShadowOffset;
            out << YAML::Key << "ShadowColor" << YAML::Value << lbl.Style.ShadowColor;
            out << YAML::Key << "LetterSpacing" << YAML::Value << lbl.Style.LetterSpacing;
            out << YAML::Key << "LineHeight" << YAML::Value << lbl.Style.LineHeight;
            out << YAML::Key << "HorizontalAlignment" << YAML::Value << (int)lbl.Style.HorizontalAlignment;
            out << YAML::Key << "VerticalAlignment" << YAML::Value << (int)lbl.Style.VerticalAlignment;
            out << YAML::EndMap;
            
            out << YAML::EndMap;
        }

        if (entity.HasComponent<ButtonControl>())
        {
            out << YAML::Key << "ButtonControl";
            auto &btn = entity.GetComponent<ButtonControl>();
            out << YAML::BeginMap;
            out << YAML::Key << "Label" << YAML::Value << btn.Label;
            out << YAML::Key << "Interactable" << YAML::Value << btn.IsInteractable;
            
            out << YAML::Key << "Style" << YAML::BeginMap;
            out << YAML::Key << "BackgroundColor" << YAML::Value << btn.Style.BackgroundColor;
            out << YAML::Key << "HoverColor" << YAML::Value << btn.Style.HoverColor;
            out << YAML::Key << "PressedColor" << YAML::Value << btn.Style.PressedColor;
            out << YAML::Key << "Rounding" << YAML::Value << btn.Style.Rounding;
            out << YAML::Key << "BorderSize" << YAML::Value << btn.Style.BorderSize;
            out << YAML::Key << "BorderColor" << YAML::Value << btn.Style.BorderColor;
            out << YAML::Key << "Padding" << YAML::Value << btn.Style.Padding;
            out << YAML::EndMap;

            out << YAML::Key << "Text" << YAML::BeginMap;
            out << YAML::Key << "FontName" << YAML::Value << btn.Text.FontName;
            out << YAML::Key << "FontSize" << YAML::Value << btn.Text.FontSize;
            out << YAML::Key << "TextColor" << YAML::Value << btn.Text.TextColor;
            out << YAML::Key << "LetterSpacing" << YAML::Value << btn.Text.LetterSpacing;
            out << YAML::Key << "LineHeight" << YAML::Value << btn.Text.LineHeight;
            out << YAML::Key << "HorizontalAlignment" << YAML::Value << (int)btn.Text.HorizontalAlignment;
            out << YAML::Key << "VerticalAlignment" << YAML::Value << (int)btn.Text.VerticalAlignment;
            out << YAML::EndMap;

            out << YAML::EndMap;
        }

        if (entity.HasComponent<PanelControl>())
        {
            out << YAML::Key << "PanelControl";
            auto &pnl = entity.GetComponent<PanelControl>();
            out << YAML::BeginMap;
            out << YAML::Key << "FullScreen" << YAML::Value << pnl.FullScreen;
            out << YAML::Key << "TexturePath" << YAML::Value << pnl.TexturePath;
            
            out << YAML::Key << "Style" << YAML::BeginMap;
            out << YAML::Key << "BackgroundColor" << YAML::Value << pnl.Style.BackgroundColor;
            out << YAML::Key << "Rounding" << YAML::Value << pnl.Style.Rounding;
            out << YAML::Key << "BorderSize" << YAML::Value << pnl.Style.BorderSize;
            out << YAML::Key << "BorderColor" << YAML::Value << pnl.Style.BorderColor;
            out << YAML::EndMap;

            out << YAML::EndMap;
        }

        if (entity.HasComponent<SliderControl>())
        {
            out << YAML::Key << "SliderControl";
            auto &sl = entity.GetComponent<SliderControl>();
            out << YAML::BeginMap;
            out << YAML::Key << "Label" << YAML::Value << sl.Label;
            out << YAML::Key << "Value" << YAML::Value << sl.Value;
            out << YAML::Key << "Min" << YAML::Value << sl.Min;
            out << YAML::Key << "Max" << YAML::Value << sl.Max;

            out << YAML::Key << "Text" << YAML::BeginMap;
            out << YAML::Key << "FontName" << YAML::Value << sl.Text.FontName;
            out << YAML::Key << "FontSize" << YAML::Value << sl.Text.FontSize;
            out << YAML::Key << "TextColor" << YAML::Value << sl.Text.TextColor;
            out << YAML::Key << "HorizontalAlignment" << YAML::Value << (int)sl.Text.HorizontalAlignment;
            out << YAML::Key << "VerticalAlignment" << YAML::Value << (int)sl.Text.VerticalAlignment;
            out << YAML::EndMap;

            out << YAML::EndMap;
        }

        if (entity.HasComponent<CheckboxControl>())
        {
            out << YAML::Key << "CheckboxControl";
            auto &cb = entity.GetComponent<CheckboxControl>();
            out << YAML::BeginMap;
            out << YAML::Key << "Label" << YAML::Value << cb.Label;
            out << YAML::Key << "Checked" << YAML::Value << cb.Checked;

            out << YAML::Key << "Text" << YAML::BeginMap;
            out << YAML::Key << "FontName" << YAML::Value << cb.Text.FontName;
            out << YAML::Key << "FontSize" << YAML::Value << cb.Text.FontSize;
            out << YAML::Key << "TextColor" << YAML::Value << cb.Text.TextColor;
            out << YAML::Key << "HorizontalAlignment" << YAML::Value << (int)cb.Text.HorizontalAlignment;
            out << YAML::Key << "VerticalAlignment" << YAML::Value << (int)cb.Text.VerticalAlignment;
            out << YAML::EndMap;

            out << YAML::EndMap;
        }
    }

    // === Deserialization ===

    void ComponentSerializer::DeserializeTag(Entity entity, YAML::Node node)
    {
        auto tagComponent = node["TagComponent"];
        if (tagComponent && tagComponent["Tag"])
        {
            auto tagNode = tagComponent["Tag"];
            if (tagNode.IsScalar())
                entity.GetComponent<TagComponent>().Tag = tagNode.as<std::string>();
        }
    }

    void ComponentSerializer::DeserializeTransform(Entity entity, YAML::Node node)
    {
        auto transformComponent = node["TransformComponent"];
        if (transformComponent)
        {
            auto &tc = entity.GetComponent<TransformComponent>();
            if (transformComponent["Translation"]) tc.Translation = transformComponent["Translation"].as<Vector3>();
            if (transformComponent["Rotation"]) tc.Rotation = transformComponent["Rotation"].as<Vector3>();
            tc.RotationQuat = QuaternionFromEuler(tc.Rotation.x, tc.Rotation.y, tc.Rotation.z);
            if (transformComponent["Scale"]) tc.Scale = transformComponent["Scale"].as<Vector3>();
        }
    }

    void ComponentSerializer::DeserializeModel(Entity entity, YAML::Node node)
    {
        auto modelComponent = node["ModelComponent"];
        if (modelComponent)
        {
            auto &mc = entity.AddComponent<ModelComponent>();
            if (modelComponent["ModelPath"] && modelComponent["ModelPath"].IsScalar())
                mc.ModelPath = modelComponent["ModelPath"].as<std::string>();

            auto materials = modelComponent["Materials"];
            if (materials && materials.IsSequence())
            {
                for (auto slotNode : materials)
                {
                    MaterialSlot slot;
                    slot.Name = slotNode["Name"].as<std::string>();
                    slot.Index = slotNode["Index"].as<int>();
                    if (slotNode["Target"]) slot.Target = (MaterialSlotTarget)slotNode["Target"].as<int>();

                    auto mat = slotNode["Material"];
                    slot.Material.AlbedoColor = mat["AlbedoColor"].as<Color>();
                    slot.Material.AlbedoPath = mat["AlbedoPath"].as<std::string>();
                    if (mat["OverrideAlbedo"]) slot.Material.OverrideAlbedo = mat["OverrideAlbedo"].as<bool>();
                    if (mat["NormalMapPath"]) slot.Material.NormalMapPath = mat["NormalMapPath"].as<std::string>();
                    if (mat["OverrideNormal"]) slot.Material.OverrideNormal = mat["OverrideNormal"].as<bool>();
                    if (mat["MetallicRoughnessPath"]) slot.Material.MetallicRoughnessPath = mat["MetallicRoughnessPath"].as<std::string>();
                    if (mat["OverrideMetallicRoughness"]) slot.Material.OverrideMetallicRoughness = mat["OverrideMetallicRoughness"].as<bool>();
                    if (mat["EmissivePath"]) slot.Material.EmissivePath = mat["EmissivePath"].as<std::string>();
                    if (mat["OverrideEmissive"]) slot.Material.OverrideEmissive = mat["OverrideEmissive"].as<bool>();
                    if (mat["Metalness"] && mat["Metalness"].IsScalar()) slot.Material.Metalness = mat["Metalness"].as<float>();
                    if (mat["Roughness"] && mat["Roughness"].IsScalar()) slot.Material.Roughness = mat["Roughness"].as<float>();

                    mc.Materials.push_back(slot);
                }
                mc.MaterialsInitialized = true;
            }
        }
    }

    void ComponentSerializer::DeserializeSpawn(Entity entity, YAML::Node node)
    {
        auto spawnComponent = node["SpawnComponent"];
        if (spawnComponent)
        {
            auto &sc = entity.AddComponent<SpawnComponent>();
            if (spawnComponent["IsActive"]) sc.IsActive = spawnComponent["IsActive"].as<bool>();
            if (spawnComponent["ZoneSize"]) sc.ZoneSize = spawnComponent["ZoneSize"].as<Vector3>();
        }
    }

    void ComponentSerializer::DeserializeCollider(Entity entity, YAML::Node node)
    {
        auto colliderComponent = node["ColliderComponent"];
        if (colliderComponent)
        {
            auto &cc = entity.AddComponent<ColliderComponent>();
            if (colliderComponent["Type"]) cc.Type = (ColliderType)colliderComponent["Type"].as<int>();
            if (colliderComponent["Enabled"]) cc.Enabled = colliderComponent["Enabled"].as<bool>();
            if (colliderComponent["Offset"]) cc.Offset = colliderComponent["Offset"].as<Vector3>();
            if (colliderComponent["Size"]) cc.Size = colliderComponent["Size"].as<Vector3>();
            if (colliderComponent["AutoCalculate"]) cc.AutoCalculate = colliderComponent["AutoCalculate"].as<bool>();
            if (colliderComponent["ModelPath"]) cc.ModelPath = colliderComponent["ModelPath"].as<std::string>();
        }
    }

    void ComponentSerializer::DeserializePointLight(Entity entity, YAML::Node node)
    {
        auto pointLightComponent = node["PointLightComponent"];
        if (pointLightComponent)
        {
            auto &lc = entity.AddComponent<PointLightComponent>();
            if (pointLightComponent["LightColor"]) lc.LightColor = pointLightComponent["LightColor"].as<Color>();
            if (pointLightComponent["Intensity"] && pointLightComponent["Intensity"].IsScalar()) lc.Intensity = pointLightComponent["Intensity"].as<float>();
            if (pointLightComponent["Radius"] && pointLightComponent["Radius"].IsScalar()) lc.Radius = pointLightComponent["Radius"].as<float>();
        }
    }

    void ComponentSerializer::DeserializeSpotLight(Entity entity, YAML::Node node)
    {
        auto spotLightComponent = node["SpotLightComponent"];
        if (spotLightComponent)
        {
            auto &lc = entity.AddComponent<SpotLightComponent>();
            if (spotLightComponent["LightColor"]) lc.LightColor = spotLightComponent["LightColor"].as<Color>();
            if (spotLightComponent["Intensity"] && spotLightComponent["Intensity"].IsScalar()) lc.Intensity = spotLightComponent["Intensity"].as<float>();
            if (spotLightComponent["Range"] && spotLightComponent["Range"].IsScalar()) lc.Range = spotLightComponent["Range"].as<float>();
            if (spotLightComponent["InnerCutoff"] && spotLightComponent["InnerCutoff"].IsScalar()) lc.InnerCutoff = spotLightComponent["InnerCutoff"].as<float>();
            if (spotLightComponent["OuterCutoff"] && spotLightComponent["OuterCutoff"].IsScalar()) lc.OuterCutoff = spotLightComponent["OuterCutoff"].as<float>();
        }
    }

    void ComponentSerializer::DeserializeRigidBody(Entity entity, YAML::Node node)
    {
        auto rigidBodyComponent = node["RigidBodyComponent"];
        if (rigidBodyComponent)
        {
            auto &rb = entity.AddComponent<RigidBodyComponent>();
            if (rigidBodyComponent["Velocity"]) rb.Velocity = rigidBodyComponent["Velocity"].as<Vector3>();
            if (rigidBodyComponent["UseGravity"]) rb.UseGravity = rigidBodyComponent["UseGravity"].as<bool>();
            if (rigidBodyComponent["IsKinematic"]) rb.IsKinematic = rigidBodyComponent["IsKinematic"].as<bool>();
            if (rigidBodyComponent["Mass"] && rigidBodyComponent["Mass"].IsScalar()) rb.Mass = rigidBodyComponent["Mass"].as<float>();
        }
    }

    void ComponentSerializer::DeserializePlayer(Entity entity, YAML::Node node)
    {
        auto playerComponent = node["PlayerComponent"];
        if (playerComponent)
        {
            auto &pc = entity.AddComponent<PlayerComponent>();
            if (playerComponent["MovementSpeed"] && playerComponent["MovementSpeed"].IsScalar()) pc.MovementSpeed = playerComponent["MovementSpeed"].as<float>();
            if (playerComponent["LookSensitivity"] && playerComponent["LookSensitivity"].IsScalar()) pc.LookSensitivity = playerComponent["LookSensitivity"].as<float>();
            if (playerComponent["CameraYaw"] && playerComponent["CameraYaw"].IsScalar()) pc.CameraYaw = playerComponent["CameraYaw"].as<float>();
            if (playerComponent["CameraPitch"] && playerComponent["CameraPitch"].IsScalar()) pc.CameraPitch = playerComponent["CameraPitch"].as<float>();
            if (playerComponent["CameraDistance"] && playerComponent["CameraDistance"].IsScalar()) pc.CameraDistance = playerComponent["CameraDistance"].as<float>();
            if (playerComponent["JumpForce"] && playerComponent["JumpForce"].IsScalar()) pc.JumpForce = playerComponent["JumpForce"].as<float>();
        }
    }

    void ComponentSerializer::DeserializeSceneTransition(Entity entity, YAML::Node node)
    {
        auto sceneTransitionNode = node["SceneTransitionComponent"];
        if (sceneTransitionNode)
        {
            auto &stc = entity.AddComponent<SceneTransitionComponent>();
            if (sceneTransitionNode["TargetScenePath"]) stc.TargetScenePath = sceneTransitionNode["TargetScenePath"].as<std::string>();
            if (sceneTransitionNode["Triggered"]) stc.Triggered = sceneTransitionNode["Triggered"].as<bool>();
        }
    }

    void ComponentSerializer::DeserializeBillboard(Entity entity, YAML::Node node)
    {
        auto billboardNode = node["BillboardComponent"];
        if (billboardNode)
        {
            auto &bc = entity.AddComponent<BillboardComponent>();
            if (billboardNode["TexturePath"] && billboardNode["TexturePath"].IsScalar()) bc.TexturePath = billboardNode["TexturePath"].as<std::string>();
            if (billboardNode["Tint"]) bc.Tint = billboardNode["Tint"].as<Color>();
            if (billboardNode["Size"] && billboardNode["Size"].IsScalar()) bc.Size = billboardNode["Size"].as<float>();
        }
    }

    void ComponentSerializer::DeserializeNavigation(Entity entity, YAML::Node node)
    {
        auto navigationNode = node["NavigationComponent"];
        if (navigationNode)
        {
            auto &nc = entity.AddComponent<NavigationComponent>();
            if (navigationNode["IsDefaultFocus"]) nc.IsDefaultFocus = navigationNode["IsDefaultFocus"].as<bool>();
        }
    }

    void ComponentSerializer::DeserializeShader(Entity entity, YAML::Node node)
    {
        auto shaderNode = node["ShaderComponent"];
        if (shaderNode)
        {
            auto &sc = entity.AddComponent<ShaderComponent>();
            if (shaderNode["ShaderPath"]) sc.ShaderPath = shaderNode["ShaderPath"].as<std::string>();
            if (shaderNode["Enabled"]) sc.Enabled = shaderNode["Enabled"].as<bool>();

            auto uniforms = shaderNode["Uniforms"];
            if (uniforms && uniforms.IsSequence())
            {
                for (auto uNode : uniforms)
                {
                    ShaderUniform u;
                    u.Name = uNode["Name"].as<std::string>();
                    u.Type = uNode["Type"].as<int>();
                    auto vals = uNode["Value"].as<std::vector<float>>();
                    for (int i = 0; i < 4 && i < vals.size(); ++i) u.Value[i] = vals[i];
                    sc.Uniforms.push_back(u);
                }
            }
        }
    }

    void ComponentSerializer::DeserializeAnimation(Entity entity, YAML::Node node)
    {
        auto animationComponent = node["AnimationComponent"];
        if (animationComponent)
        {
            auto &anim = entity.AddComponent<AnimationComponent>();
            if (animationComponent["AnimationPath"] && animationComponent["AnimationPath"].IsScalar()) anim.AnimationPath = animationComponent["AnimationPath"].as<std::string>();
            if (animationComponent["CurrentAnimationIndex"] && animationComponent["CurrentAnimationIndex"].IsScalar()) anim.CurrentAnimationIndex = animationComponent["CurrentAnimationIndex"].as<int>();
            if (animationComponent["IsLooping"]) anim.IsLooping = animationComponent["IsLooping"].as<bool>();
            if (animationComponent["IsPlaying"]) anim.IsPlaying = animationComponent["IsPlaying"].as<bool>();
        }
    }

    void ComponentSerializer::DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask)
    {
        auto hierarchyComponent = node["HierarchyComponent"];
        if (hierarchyComponent)
        {
            outTask.entity = entity;
            outTask.parent = hierarchyComponent["Parent"].as<uint64_t>();
            auto children = hierarchyComponent["Children"];
            if (children)
            {
                for (auto child : children) outTask.children.push_back(child.as<uint64_t>());
            }
        }
    }

    void ComponentSerializer::DeserializeAudio(Entity entity, YAML::Node node)
    {
        auto audioComponent = node["AudioComponent"];
        if (audioComponent)
        {
            auto &ac = entity.AddComponent<AudioComponent>();
            if (audioComponent["SoundPath"] && audioComponent["SoundPath"].IsScalar()) ac.SoundPath = audioComponent["SoundPath"].as<std::string>();
            if (audioComponent["Volume"] && audioComponent["Volume"].IsScalar()) ac.Volume = audioComponent["Volume"].as<float>();
            if (audioComponent["Pitch"] && audioComponent["Pitch"].IsScalar()) ac.Pitch = audioComponent["Pitch"].as<float>();
            if (audioComponent["Loop"]) ac.Loop = audioComponent["Loop"].as<bool>();
            if (audioComponent["PlayOnStart"]) ac.PlayOnStart = audioComponent["PlayOnStart"].as<bool>();
        }
    }

    void ComponentSerializer::DeserializeCamera(Entity entity, YAML::Node node)
    {
        auto cameraComponent = node["CameraComponent"];
        if (cameraComponent)
        {
            auto &cc = entity.AddComponent<CameraComponent>();
            if (cameraComponent["Fov"] && cameraComponent["Fov"].IsScalar()) cc.Fov = cameraComponent["Fov"].as<float>();
            if (cameraComponent["Offset"]) cc.Offset = cameraComponent["Offset"].as<Vector3>();
        }
    }

    void ComponentSerializer::DeserializeNativeScript(Entity entity, YAML::Node node)
    {
        if (node["NativeScriptComponent"])
        {
            auto &nsc = entity.AddComponent<NativeScriptComponent>();
            auto scripts = node["NativeScriptComponent"]["Scripts"];
            if (scripts && scripts.IsSequence())
            {
                for (auto script : scripts)
                {
                    std::string scriptName;
                    if (script.IsMap())
                    {
                        auto snNode = script["ScriptName"];
                        if (snNode && snNode.IsScalar())
                        {
                            scriptName = snNode.as<std::string>();
                        }
                    }
                    else if (script.IsScalar())
                    {
                        scriptName = script.as<std::string>();
                    }

                    if (!scriptName.empty())
                    {
                        ScriptRegistry::AddScript(scriptName, nsc);
                    }
                }
            }
        }
    }

    void ComponentSerializer::DeserializeUI(Entity entity, YAML::Node node)
    {
        auto controlNode = node["ControlComponent"];
        if (controlNode)
        {
            auto &cc = entity.AddComponent<ControlComponent>();
            if (controlNode["IsActive"]) cc.IsActive = controlNode["IsActive"].as<bool>();
            if (controlNode["HiddenInHierarchy"]) cc.HiddenInHierarchy = controlNode["HiddenInHierarchy"].as<bool>();
            if (controlNode["ZOrder"] && controlNode["ZOrder"].IsScalar()) cc.ZOrder = controlNode["ZOrder"].as<int>();

            auto rectNode = controlNode["RectTransform"];
            if (rectNode)
            {
                if (rectNode["AnchorMin"]) cc.Transform.AnchorMin = rectNode["AnchorMin"].as<glm::vec2>();
                if (rectNode["AnchorMax"]) cc.Transform.AnchorMax = rectNode["AnchorMax"].as<glm::vec2>();
                if (rectNode["OffsetMin"]) cc.Transform.OffsetMin = rectNode["OffsetMin"].as<glm::vec2>();
                if (rectNode["OffsetMax"]) cc.Transform.OffsetMax = rectNode["OffsetMax"].as<glm::vec2>();
                if (rectNode["Pivot"]) cc.Transform.Pivot = rectNode["Pivot"].as<glm::vec2>();
                if (rectNode["Rotation"]) cc.Transform.Rotation = rectNode["Rotation"].as<float>();
                if (rectNode["Scale"]) cc.Transform.Scale = rectNode["Scale"].as<glm::vec2>();
            }

            auto panelNode = node["PanelControl"];
            if (panelNode)
            {
                auto &pnl = entity.AddComponent<PanelControl>();
                if (panelNode["FullScreen"]) pnl.FullScreen = panelNode["FullScreen"].as<bool>();
                if (panelNode["TexturePath"] && panelNode["TexturePath"].IsScalar()) 
                    pnl.TexturePath = panelNode["TexturePath"].as<std::string>();

                auto styleNode = panelNode["Style"];
                if (styleNode)
                {
                    if (styleNode["BackgroundColor"]) pnl.Style.BackgroundColor = styleNode["BackgroundColor"].as<Color>();
                    if (styleNode["Rounding"]) pnl.Style.Rounding = styleNode["Rounding"].as<float>();
                    if (styleNode["BorderSize"]) pnl.Style.BorderSize = styleNode["BorderSize"].as<float>();
                    if (styleNode["BorderColor"]) pnl.Style.BorderColor = styleNode["BorderColor"].as<Color>();
                }
            }

            auto labelNode = node["LabelControl"];
            if (labelNode)
            {
                auto &lbl = entity.AddComponent<LabelControl>();
                if (labelNode["Text"] && labelNode["Text"].IsScalar()) 
                    lbl.Text = labelNode["Text"].as<std::string>();
                
                auto styleNode = labelNode["Style"];
                if (styleNode)
                {
                    if (styleNode["FontName"] && styleNode["FontName"].IsScalar()) 
                        lbl.Style.FontName = styleNode["FontName"].as<std::string>();
                    if (styleNode["FontSize"]) lbl.Style.FontSize = styleNode["FontSize"].as<float>();
                    if (styleNode["TextColor"]) lbl.Style.TextColor = styleNode["TextColor"].as<Color>();
                    if (styleNode["Shadow"]) lbl.Style.Shadow = styleNode["Shadow"].as<bool>();
                    if (styleNode["ShadowOffset"] && styleNode["ShadowOffset"].IsScalar()) lbl.Style.ShadowOffset = styleNode["ShadowOffset"].as<float>();
                    if (styleNode["ShadowColor"]) lbl.Style.ShadowColor = styleNode["ShadowColor"].as<Color>();
                    if (styleNode["LetterSpacing"] && styleNode["LetterSpacing"].IsScalar()) lbl.Style.LetterSpacing = styleNode["LetterSpacing"].as<float>();
                    if (styleNode["LineHeight"] && styleNode["LineHeight"].IsScalar()) lbl.Style.LineHeight = styleNode["LineHeight"].as<float>();
                    if (styleNode["HorizontalAlignment"] && styleNode["HorizontalAlignment"].IsScalar()) lbl.Style.HorizontalAlignment = (TextAlignment)styleNode["HorizontalAlignment"].as<int>();
                    if (styleNode["VerticalAlignment"] && styleNode["VerticalAlignment"].IsScalar()) lbl.Style.VerticalAlignment = (TextAlignment)styleNode["VerticalAlignment"].as<int>();
                    if (styleNode["Alignment"] && styleNode["Alignment"].IsScalar()) lbl.Style.HorizontalAlignment = (TextAlignment)styleNode["Alignment"].as<int>(); // Backward compatibility
                }
                else
                {
                    // Backward compatibility for root-level properties
                    if (labelNode["FontSize"]) lbl.Style.FontSize = labelNode["FontSize"].as<float>();
                    if (labelNode["TextColor"]) lbl.Style.TextColor = labelNode["TextColor"].as<Color>();
                    if (labelNode["Alignment"]) lbl.Style.HorizontalAlignment = (TextAlignment)labelNode["Alignment"].as<int>();
                }
            }

            auto buttonNode = node["ButtonControl"];
            if (buttonNode)
            {
                auto &btn = entity.AddComponent<ButtonControl>();
                if (buttonNode["Label"]) btn.Label = buttonNode["Label"].as<std::string>();
                if (buttonNode["Interactable"]) btn.IsInteractable = buttonNode["Interactable"].as<bool>();

                auto style = buttonNode["Style"];
                if (style)
                {
                    if (style["BackgroundColor"]) btn.Style.BackgroundColor = style["BackgroundColor"].as<Color>();
                    if (style["HoverColor"]) btn.Style.HoverColor = style["HoverColor"].as<Color>();
                    if (style["PressedColor"]) btn.Style.PressedColor = style["PressedColor"].as<Color>();
                    if (style["Rounding"]) btn.Style.Rounding = style["Rounding"].as<float>();
                    if (style["BorderSize"]) btn.Style.BorderSize = style["BorderSize"].as<float>();
                    if (style["BorderColor"]) btn.Style.BorderColor = style["BorderColor"].as<Color>();
                    if (style["Padding"]) btn.Style.Padding = style["Padding"].as<float>();
                }

                auto textNode = buttonNode["Text"];
                if (textNode)
                {
                    if (textNode["FontName"] && textNode["FontName"].IsScalar()) 
                        btn.Text.FontName = textNode["FontName"].as<std::string>();
                    if (textNode["FontSize"] && textNode["FontSize"].IsScalar()) btn.Text.FontSize = textNode["FontSize"].as<float>();
                    if (textNode["TextColor"]) btn.Text.TextColor = textNode["TextColor"].as<Color>();
                    if (textNode["LetterSpacing"] && textNode["LetterSpacing"].IsScalar()) btn.Text.LetterSpacing = textNode["LetterSpacing"].as<float>();
                    if (textNode["LineHeight"] && textNode["LineHeight"].IsScalar()) btn.Text.LineHeight = textNode["LineHeight"].as<float>();
                    if (textNode["HorizontalAlignment"] && textNode["HorizontalAlignment"].IsScalar()) btn.Text.HorizontalAlignment = (TextAlignment)textNode["HorizontalAlignment"].as<int>();
                    if (textNode["VerticalAlignment"] && textNode["VerticalAlignment"].IsScalar()) btn.Text.VerticalAlignment = (TextAlignment)textNode["VerticalAlignment"].as<int>();
                }
            }

            auto sliderNode = node["SliderControl"];
            if (sliderNode)
            {
                auto &sl = entity.AddComponent<SliderControl>();
                if (sliderNode["Label"]) sl.Label = sliderNode["Label"].as<std::string>();
                if (sliderNode["Value"] && sliderNode["Value"].IsScalar()) sl.Value = sliderNode["Value"].as<float>();
                if (sliderNode["Min"] && sliderNode["Min"].IsScalar()) sl.Min = sliderNode["Min"].as<float>();
                if (sliderNode["Max"] && sliderNode["Max"].IsScalar()) sl.Max = sliderNode["Max"].as<float>();

                auto textNode = sliderNode["Text"];
                if (textNode)
                {
                    if (textNode["FontName"] && textNode["FontName"].IsScalar()) sl.Text.FontName = textNode["FontName"].as<std::string>();
                    if (textNode["FontSize"]) sl.Text.FontSize = textNode["FontSize"].as<float>();
                    if (textNode["TextColor"]) sl.Text.TextColor = textNode["TextColor"].as<Color>();
                    if (textNode["HorizontalAlignment"]) sl.Text.HorizontalAlignment = (TextAlignment)textNode["HorizontalAlignment"].as<int>();
                    if (textNode["VerticalAlignment"]) sl.Text.VerticalAlignment = (TextAlignment)textNode["VerticalAlignment"].as<int>();
                }
            }

            auto checkboxNode = node["CheckboxControl"];
            if (checkboxNode)
            {
                auto &cb = entity.AddComponent<CheckboxControl>();
                if (checkboxNode["Label"]) cb.Label = checkboxNode["Label"].as<std::string>();
                if (checkboxNode["Value"]) cb.Checked = checkboxNode["Value"].as<bool>();
                if (checkboxNode["Checked"]) cb.Checked = checkboxNode["Checked"].as<bool>(); // Both names support

                auto textNode = checkboxNode["Text"];
                if (textNode)
                {
                    if (textNode["FontName"] && textNode["FontName"].IsScalar()) cb.Text.FontName = textNode["FontName"].as<std::string>();
                    if (textNode["FontSize"]) cb.Text.FontSize = textNode["FontSize"].as<float>();
                    if (textNode["TextColor"]) cb.Text.TextColor = textNode["TextColor"].as<Color>();
                    if (textNode["HorizontalAlignment"]) cb.Text.HorizontalAlignment = (TextAlignment)textNode["HorizontalAlignment"].as<int>();
                    if (textNode["VerticalAlignment"]) cb.Text.VerticalAlignment = (TextAlignment)textNode["VerticalAlignment"].as<int>();
                }
            }
        }
    }
}
