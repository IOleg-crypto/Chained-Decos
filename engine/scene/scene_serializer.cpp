#include "scene_serializer.h"
#include "components.h"
#include "engine/core/log.h"
#include "engine/core/yaml.h"
#include "engine/physics/bvh/bvh.h"
// Removed redundant include: engine/graphics/asset_manager.h
// Removed redundant include: engine/graphics/render.h
#include "engine/graphics/model_asset.h"
#include "scene.h"
#include "script_registry.h"

#include "fstream"
#include "yaml-cpp/yaml.h"

namespace CHEngine
{
    SceneSerializer::SceneSerializer(Scene *scene) : m_Scene(scene)
    {
    }

    static void SerializeEntity(YAML::Emitter &out, Entity entity)
    {
        out << YAML::BeginMap; // Entity
        if (entity.HasComponent<IDComponent>())
        {
            out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entity.GetComponent<IDComponent>().ID;
        }
        else
        {
            out << YAML::Key << "Entity" << YAML::Value << 0;
        }

        if (entity.HasComponent<TagComponent>())
        {
            out << YAML::Key << "TagComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "Tag" << YAML::Value << entity.GetComponent<TagComponent>().Tag;
            out << YAML::EndMap;
        }

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

        // MaterialComponent is deprecated and merged into ModelComponent per USER request

        if (entity.HasComponent<SpawnComponent>())
        {
            out << YAML::Key << "SpawnComponent";
            auto &sc = entity.GetComponent<SpawnComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "IsActive" << YAML::Value << sc.IsActive;
            out << YAML::Key << "ZoneSize" << YAML::Value << sc.ZoneSize;
            out << YAML::EndMap;
        }

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

        if (entity.HasComponent<PointLightComponent>())
        {
            out << YAML::Key << "PointLightComponent";
            out << YAML::BeginMap;
            auto &plc = entity.GetComponent<PointLightComponent>();
            out << YAML::Key << "LightColor" << YAML::Value << plc.LightColor;
            out << YAML::Key << "Radiance" << YAML::Value << plc.Radiance;
            out << YAML::Key << "Radius" << YAML::Value << plc.Radius;
            out << YAML::Key << "Falloff" << YAML::Value << plc.Falloff;
            out << YAML::EndMap;
        }

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

        if (entity.HasComponent<NativeScriptComponent>())
        {
            out << YAML::Key << "NativeScriptComponent";
            auto &nsc = entity.GetComponent<NativeScriptComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
            for (const auto &script : nsc.Scripts)
            {
                out << script.ScriptName;
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }

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

        if (entity.HasComponent<CameraComponent>())
        {
            out << YAML::Key << "CameraComponent";
            auto &cc = entity.GetComponent<CameraComponent>();
            out << YAML::BeginMap;
            out << YAML::Key << "Fov" << YAML::Value << cc.Fov;
            out << YAML::Key << "Offset" << YAML::Value << cc.Offset;
            out << YAML::EndMap;
        }

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
            out << YAML::Key << "RectCoordinates" << YAML::Value << cc.Transform.RectCoordinates;
            out << YAML::EndMap;

            out << YAML::EndMap;
        }

        if (entity.HasComponent<LabelControl>())
        {
            out << YAML::Key << "LabelControl";
            auto &lbl = entity.GetComponent<LabelControl>();
            out << YAML::BeginMap;
            out << YAML::Key << "Text" << YAML::Value << lbl.Text;
            out << YAML::Key << "FontSize" << YAML::Value << lbl.Style.FontSize;
            out << YAML::Key << "TextColor" << YAML::Value << lbl.Style.TextColor;
            out << YAML::Key << "Shadow" << YAML::Value << lbl.Style.Shadow;
            out << YAML::Key << "ShadowOffset" << YAML::Value << lbl.Style.ShadowOffset;
            out << YAML::Key << "ShadowColor" << YAML::Value << lbl.Style.ShadowColor;
            out << YAML::Key << "LetterSpacing" << YAML::Value << lbl.Style.LetterSpacing;
            out << YAML::Key << "Alignment" << YAML::Value << (int)lbl.Style.Alignment;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<ButtonControl>())
        {
            out << YAML::Key << "ButtonControl";
            auto &btn = entity.GetComponent<ButtonControl>();
            out << YAML::BeginMap;
            out << YAML::Key << "Label" << YAML::Value << btn.Label;
            out << YAML::Key << "Interactable" << YAML::Value << btn.IsInteractable;

            out << YAML::Key << "Style";
            out << YAML::BeginMap;
            out << YAML::Key << "BackgroundColor" << YAML::Value << btn.Style.BackgroundColor;
            out << YAML::Key << "HoverColor" << YAML::Value << btn.Style.HoverColor;
            out << YAML::Key << "PressedColor" << YAML::Value << btn.Style.PressedColor;
            out << YAML::EndMap;

            out << YAML::Key << "Action" << YAML::Value << (int)btn.Action;
            out << YAML::Key << "TargetScene" << YAML::Value << btn.TargetScene;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<PanelControl>())
        {
            out << YAML::Key << "PanelControl";
            auto &pnl = entity.GetComponent<PanelControl>();
            out << YAML::BeginMap;
            out << YAML::Key << "Color" << YAML::Value << pnl.Style.BackgroundColor;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<SliderControl>())
        {
            out << YAML::Key << "SliderControl";
            auto &sl = entity.GetComponent<SliderControl>();
            out << YAML::BeginMap;
            out << YAML::Key << "Value" << YAML::Value << sl.Value;
            out << YAML::Key << "Min" << YAML::Value << sl.Min;
            out << YAML::Key << "Max" << YAML::Value << sl.Max;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<CheckboxControl>())
        {
            out << YAML::Key << "CheckboxControl";
            auto &cb = entity.GetComponent<CheckboxControl>();
            out << YAML::BeginMap;
            out << YAML::Key << "Checked" << YAML::Value << cb.Checked;
            out << YAML::EndMap;
        }
        out << YAML::EndMap; // Entity
    }

    std::string SceneSerializer::SerializeToString()
    {
        if (!m_Scene)
            return "";

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << "Untitled";

        // Serialize Background Settings
        out << YAML::Key << "Background";
        out << YAML::BeginMap;
        out << YAML::Key << "Mode" << YAML::Value << (int)m_Scene->GetBackgroundMode();
        out << YAML::Key << "Color" << YAML::Value << m_Scene->GetBackgroundColor();
        out << YAML::Key << "TexturePath" << YAML::Value << m_Scene->GetBackgroundTexturePath();
        out << YAML::EndMap;

        // Serialize Environment
        if (m_Scene->m_Settings.Environment)
        {
            out << YAML::Key << "EnvironmentPath" << YAML::Value << m_Scene->m_Settings.Environment->GetPath();
        }

        // Keep legacy Skybox for now to avoid breaking existing scenes
        {
            auto &sc = m_Scene->GetSkybox();
            auto &skybox = m_Scene->m_Settings.Skybox;
            out << YAML::Key << "Skybox";
            out << YAML::BeginMap;
            out << YAML::Key << "TexturePath" << YAML::Value << skybox.TexturePath;
            out << YAML::Key << "Exposure" << YAML::Value << skybox.Exposure;
            out << YAML::Key << "Brightness" << YAML::Value << skybox.Brightness;
            out << YAML::Key << "Contrast" << YAML::Value << skybox.Contrast;
            out << YAML::EndMap;
        }

        out << YAML::Key << "Canvas" << YAML::BeginMap;
        out << YAML::Key << "ReferenceResolution" << YAML::Value << m_Scene->m_Settings.Canvas.ReferenceResolution;
        out << YAML::Key << "ScaleMode" << YAML::Value << (int)m_Scene->m_Settings.Canvas.ScaleMode;
        out << YAML::Key << "MatchWidthOrHeight" << YAML::Value << m_Scene->m_Settings.Canvas.MatchWidthOrHeight;
        out << YAML::EndMap;

        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        m_Scene->GetRegistry().view<IDComponent>().each(
            [&](auto entityID, auto &id)
            {
                Entity entity = {entityID, m_Scene};
                SerializeEntity(out, entity);
            });

        out << YAML::EndSeq;
        out << YAML::EndMap;

        return std::string(out.c_str());
    }

    bool SceneSerializer::Serialize(const std::string &filepath)
    {
        std::string yaml = SerializeToString();
        std::ofstream fout(filepath);
        if (fout.is_open())
        {
            fout << yaml;
            CH_CORE_INFO("Scene saved successfully to: {}", filepath.c_str());
            return true;
        }
        else
        {
            CH_CORE_ERROR("Failed to save scene to: {}", filepath.c_str());
            return false;
        }
    }

    bool SceneSerializer::Deserialize(const std::string &filepath)
    {
        std::ifstream stream(filepath);
        if (!stream.is_open())
        {
            CH_CORE_ERROR("Failed to open scene file: {}", filepath.c_str());
            return false;
        }

        std::stringstream strStream;
        strStream << stream.rdbuf();

        return DeserializeFromString(strStream.str());
    }

    bool SceneSerializer::DeserializeFromString(const std::string &yaml)
    {
        YAML::Node data = YAML::Load(yaml);
        if (!data["Scene"])
            return false;

        std::string sceneName = data["Scene"].as<std::string>();
        CH_CORE_INFO("Deserializing scene '{}'", sceneName.c_str());

        // Unified scene - no Type needed

        // Deserialize Scene Settings
        if (data["Background"])
        {
            auto bg = data["Background"];
            if (bg["Mode"])
                m_Scene->m_Settings.Mode = (BackgroundMode)bg["Mode"].as<int>();
            if (bg["Color"])
                m_Scene->m_Settings.BackgroundColor = bg["Color"].as<Color>();
            if (bg["TexturePath"])
                m_Scene->m_Settings.BackgroundTexturePath = bg["TexturePath"].as<std::string>();
        }

        if (data["EnvironmentPath"])
        {
            // m_Scene->m_Settings.Environment =
            // AssetManager::LoadEnvironment(data["EnvironmentPath"].as<std::string>());
        }

        if (data["Skybox"])
        {
            auto skybox = data["Skybox"];
            auto &skyboxComp = m_Scene->m_Settings.Skybox;
            if (skybox["TexturePath"])
                skyboxComp.TexturePath = skybox["TexturePath"].as<std::string>();
            if (skybox["Exposure"])
                skyboxComp.Exposure = skybox["Exposure"].as<float>();
            if (skybox["Brightness"])
                skyboxComp.Brightness = skybox["Brightness"].as<float>();
            if (skybox["Contrast"])
                skyboxComp.Contrast = skybox["Contrast"].as<float>();
        }

        if (data["Canvas"])
        {
            auto canvas = data["Canvas"];
            if (canvas["ReferenceResolution"])
                m_Scene->m_Settings.Canvas.ReferenceResolution = canvas["ReferenceResolution"].as<glm::vec2>();
            if (canvas["ScaleMode"])
                m_Scene->m_Settings.Canvas.ScaleMode = (CanvasScaleMode)canvas["ScaleMode"].as<int>();
            if (canvas["MatchWidthOrHeight"])
                m_Scene->m_Settings.Canvas.MatchWidthOrHeight = canvas["MatchWidthOrHeight"].as<float>();
        }

        auto entities = data["Entities"];
        if (entities)
        {
            struct HierarchyTask
            {
                Entity entity;
                uint64_t parent;
                std::vector<uint64_t> children;
            };
            std::vector<HierarchyTask> hierarchyTasks;

            std::set<uint64_t> seenUUIDs;
            for (auto entity : entities)
            {
                uint64_t uuid = entity["Entity"].as<uint64_t>();

                // Collision check
                if (seenUUIDs.count(uuid))
                {
                    uint64_t oldUUID = uuid;
                    uuid = UUID(); // Generate new one
                    CH_CORE_WARN("SceneSerializer: Duplicate UUID {0} found! Regenerated as {1}", oldUUID, uuid);
                }
                seenUUIDs.insert(uuid);

                std::string name;
                auto tagComponent = entity["TagComponent"];
                if (tagComponent)
                    name = tagComponent["Tag"].as<std::string>();

                CH_CORE_TRACE("Deserialized entity with ID = {}, name = {}", uuid, name.c_str());

                Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

                auto transformComponent = entity["TransformComponent"];
                if (transformComponent)
                {
                    auto &tc = deserializedEntity.GetComponent<TransformComponent>();
                    tc.Translation = transformComponent["Translation"].as<Vector3>();
                    tc.Rotation = transformComponent["Rotation"].as<Vector3>();
                    tc.RotationQuat = QuaternionFromEuler(tc.Rotation.x, tc.Rotation.y, tc.Rotation.z);
                    tc.Scale = transformComponent["Scale"].as<Vector3>();
                }

                auto modelComponent = entity["ModelComponent"];
                if (modelComponent)
                {
                    auto &mc = deserializedEntity.AddComponent<ModelComponent>();
                    mc.ModelPath = modelComponent["ModelPath"].as<std::string>();

                    // Manual refresh to ensure assets are loaded even if path was set after signal
                    m_Scene->OnModelComponentAdded(m_Scene->GetRegistry(), deserializedEntity);

                    auto materials = modelComponent["Materials"];
                    if (materials && materials.IsSequence())
                    {
                        for (auto slotNode : materials)
                        {
                            MaterialSlot slot;
                            slot.Name = slotNode["Name"].as<std::string>();
                            slot.Index = slotNode["Index"].as<int>();
                            if (slotNode["Target"])
                                slot.Target = (MaterialSlotTarget)slotNode["Target"].as<int>();

                            auto mat = slotNode["Material"];
                            slot.Material.AlbedoColor = mat["AlbedoColor"].as<Color>();
                            slot.Material.AlbedoPath = mat["AlbedoPath"].as<std::string>();
                            if (mat["OverrideAlbedo"])
                                slot.Material.OverrideAlbedo = mat["OverrideAlbedo"].as<bool>();
                            if (mat["NormalMapPath"])
                                slot.Material.NormalMapPath = mat["NormalMapPath"].as<std::string>();
                            if (mat["OverrideNormal"])
                                slot.Material.OverrideNormal = mat["OverrideNormal"].as<bool>();
                            if (mat["MetallicRoughnessPath"])
                                slot.Material.MetallicRoughnessPath = mat["MetallicRoughnessPath"].as<std::string>();
                            if (mat["OverrideMetallicRoughness"])
                                slot.Material.OverrideMetallicRoughness = mat["OverrideMetallicRoughness"].as<bool>();
                            if (mat["EmissivePath"])
                                slot.Material.EmissivePath = mat["EmissivePath"].as<std::string>();
                            if (mat["OverrideEmissive"])
                                slot.Material.OverrideEmissive = mat["OverrideEmissive"].as<bool>();
                            if (mat["Metalness"])
                                slot.Material.Metalness = mat["Metalness"].as<float>();
                            if (mat["Roughness"])
                                slot.Material.Roughness = mat["Roughness"].as<float>();

                            mc.Materials.push_back(slot);
                        }
                        mc.MaterialsInitialized = true;
                    }
                }

                auto spawnComponent = entity["SpawnComponent"];
                if (spawnComponent)
                {
                    auto &sc = deserializedEntity.AddComponent<SpawnComponent>();
                    if (spawnComponent["IsActive"])
                        sc.IsActive = spawnComponent["IsActive"].as<bool>();
                    if (spawnComponent["ZoneSize"])
                        sc.ZoneSize = spawnComponent["ZoneSize"].as<Vector3>();
                }

                auto materialComponent = entity["MaterialComponent"];
                if (materialComponent)
                {
                    if (!deserializedEntity.HasComponent<MaterialComponent>())
                        deserializedEntity.AddComponent<MaterialComponent>();

                    auto &mc = deserializedEntity.GetComponent<MaterialComponent>();
                    if (materialComponent.IsSequence())
                    {
                        for (auto slotNode : materialComponent)
                        {
                            MaterialSlot slot;
                            slot.Name = slotNode["Name"].as<std::string>();
                            slot.Index = slotNode["Index"].as<int>();
                            if (slotNode["Target"])
                                slot.Target = (MaterialSlotTarget)slotNode["Target"].as<int>();

                            auto mat = slotNode["Material"];
                            slot.Material.AlbedoColor = mat["AlbedoColor"].as<Color>();
                            slot.Material.AlbedoPath = mat["AlbedoPath"].as<std::string>();
                            if (mat["OverrideAlbedo"])
                                slot.Material.OverrideAlbedo = mat["OverrideAlbedo"].as<bool>();
                            if (mat["NormalMapPath"])
                                slot.Material.NormalMapPath = mat["NormalMapPath"].as<std::string>();
                            if (mat["OverrideNormal"])
                                slot.Material.OverrideNormal = mat["OverrideNormal"].as<bool>();
                            if (mat["MetallicRoughnessPath"])
                                slot.Material.MetallicRoughnessPath = mat["MetallicRoughnessPath"].as<std::string>();
                            if (mat["OverrideMetallicRoughness"])
                                slot.Material.OverrideMetallicRoughness = mat["OverrideMetallicRoughness"].as<bool>();
                            if (mat["EmissivePath"])
                                slot.Material.EmissivePath = mat["EmissivePath"].as<std::string>();
                            if (mat["OverrideEmissive"])
                                slot.Material.OverrideEmissive = mat["OverrideEmissive"].as<bool>();
                            if (mat["Metalness"])
                                slot.Material.Metalness = mat["Metalness"].as<float>();
                            if (mat["Roughness"])
                                slot.Material.Roughness = mat["Roughness"].as<float>();
                            if (mat["ShaderPath"])
                                slot.Material.ShaderPath = mat["ShaderPath"].as<std::string>();
                            if (mat["OverrideShader"])
                                slot.Material.OverrideShader = mat["OverrideShader"].as<bool>();

                            mc.Slots.push_back(slot);
                        }
                    }
                    else
                    {
                        // Legacy migration case already handled if it was in ModelComponent,
                        // but if there's a standalone old MaterialComponent:
                        MaterialSlot slot("Legacy Standalone", -1);
                        slot.Material.AlbedoColor = materialComponent["AlbedoColor"].as<Color>();
                        slot.Material.AlbedoPath = materialComponent["AlbedoPath"].as<std::string>();
                        mc.Slots.push_back(slot);
                    }
                }

                auto colliderComponent = entity["ColliderComponent"];
                if (colliderComponent)
                {
                    auto &cc = deserializedEntity.AddComponent<ColliderComponent>();
                    if (colliderComponent["Type"])
                        cc.Type = (ColliderType)colliderComponent["Type"].as<int>();
                    if (colliderComponent["Enabled"])
                        cc.Enabled = colliderComponent["Enabled"].as<bool>();
                    if (colliderComponent["Offset"])
                        cc.Offset = colliderComponent["Offset"].as<Vector3>();
                    if (colliderComponent["Size"])
                        cc.Size = colliderComponent["Size"].as<Vector3>();
                    if (colliderComponent["AutoCalculate"])
                        cc.AutoCalculate = colliderComponent["AutoCalculate"].as<bool>();
                    if (colliderComponent["ModelPath"])
                        cc.ModelPath = colliderComponent["ModelPath"].as<std::string>();

                    if (cc.Type == ColliderType::Mesh && !cc.ModelPath.empty())
                    {
                        // auto asset = AssetManager::Get<ModelAsset>(cc.ModelPath);
                        std::shared_ptr<ModelAsset> asset = nullptr;
                        if (asset)
                            cc.BVHRoot = asset->GetBVHCache();
                    }
                }

                auto audioComponent = entity["AudioComponent"];
                if (audioComponent)
                {
                    auto &ac = deserializedEntity.AddComponent<AudioComponent>();
                    if (audioComponent["SoundPath"])
                        ac.SoundPath = audioComponent["SoundPath"].as<std::string>();
                    if (audioComponent["Volume"])
                        ac.Volume = audioComponent["Volume"].as<float>();
                    if (audioComponent["Pitch"])
                        ac.Pitch = audioComponent["Pitch"].as<float>();
                    if (audioComponent["Loop"])
                        ac.Loop = audioComponent["Loop"].as<bool>();
                    if (audioComponent["PlayOnStart"])
                        ac.PlayOnStart = audioComponent["PlayOnStart"].as<bool>();
                }

                auto cameraComponent = entity["CameraComponent"];
                if (cameraComponent)
                {
                    auto &cc = deserializedEntity.AddComponent<CameraComponent>();
                    if (cameraComponent["Fov"])
                        cc.Fov = cameraComponent["Fov"].as<float>();
                    if (cameraComponent["Offset"])
                        cc.Offset = cameraComponent["Offset"].as<Vector3>();
                }

                auto hierarchyComponent = entity["HierarchyComponent"];
                if (hierarchyComponent)
                {
                    HierarchyTask task;
                    task.entity = deserializedEntity;
                    task.parent = hierarchyComponent["Parent"].as<uint64_t>();
                    auto children = hierarchyComponent["Children"];
                    if (children)
                    {
                        for (auto child : children)
                            task.children.push_back(child.as<uint64_t>());
                    }
                    hierarchyTasks.push_back(task);
                }

                auto nativeScriptComponent = entity["NativeScriptComponent"];
                if (nativeScriptComponent)
                {
                    auto &nsc = deserializedEntity.AddComponent<NativeScriptComponent>();
                    auto scripts = nativeScriptComponent["Scripts"];
                    if (scripts)
                    {
                        for (auto script : scripts)
                        {
                            std::string scriptName = script.as<std::string>();
                            ScriptRegistry::AddScript(scriptName, nsc);
                        }
                    }
                }

                auto animationComponent = entity["AnimationComponent"];
                if (animationComponent)
                {
                    auto &anim = deserializedEntity.AddComponent<AnimationComponent>();
                    if (animationComponent["AnimationPath"])
                        anim.AnimationPath = animationComponent["AnimationPath"].as<std::string>();
                    if (animationComponent["CurrentAnimationIndex"])
                        anim.CurrentAnimationIndex = animationComponent["CurrentAnimationIndex"].as<int>();
                    if (animationComponent["IsLooping"])
                        anim.IsLooping = animationComponent["IsLooping"].as<bool>();
                    if (animationComponent["IsPlaying"])
                        anim.IsPlaying = animationComponent["IsPlaying"].as<bool>();
                }

                auto pointLightComponent = entity["PointLightComponent"];
                if (pointLightComponent)
                {
                    auto &plc = deserializedEntity.AddComponent<PointLightComponent>();
                    plc.LightColor = pointLightComponent["LightColor"].as<Color>();
                    plc.Radiance = pointLightComponent["Radiance"].as<float>();
                    plc.Radius = pointLightComponent["Radius"].as<float>();
                    plc.Falloff = pointLightComponent["Falloff"].as<float>();
                }

                auto rigidBodyComponent = entity["RigidBodyComponent"];
                if (rigidBodyComponent)
                {
                    auto &rb = deserializedEntity.AddComponent<RigidBodyComponent>();
                    if (rigidBodyComponent["Velocity"])
                        rb.Velocity = rigidBodyComponent["Velocity"].as<Vector3>();
                    if (rigidBodyComponent["UseGravity"])
                        rb.UseGravity = rigidBodyComponent["UseGravity"].as<bool>();
                    if (rigidBodyComponent["IsKinematic"])
                        rb.IsKinematic = rigidBodyComponent["IsKinematic"].as<bool>();
                    if (rigidBodyComponent["Mass"])
                        rb.Mass = rigidBodyComponent["Mass"].as<float>();
                }

                auto controlNode = entity["ControlComponent"];
                if (controlNode)
                {
                    auto &cc = deserializedEntity.AddComponent<ControlComponent>();
                    if (controlNode["IsActive"])
                        cc.IsActive = controlNode["IsActive"].as<bool>();
                    if (controlNode["HiddenInHierarchy"])
                        cc.HiddenInHierarchy = controlNode["HiddenInHierarchy"].as<bool>();
                    if (controlNode["ZOrder"])
                        cc.ZOrder = controlNode["ZOrder"].as<int>();

                    auto rectNode = controlNode["RectTransform"];
                    if (rectNode)
                    {
                        if (rectNode["AnchorMin"]) cc.Transform.AnchorMin = rectNode["AnchorMin"].as<glm::vec2>();
                        if (rectNode["AnchorMax"]) cc.Transform.AnchorMax = rectNode["AnchorMax"].as<glm::vec2>();
                        if (rectNode["OffsetMin"]) cc.Transform.OffsetMin = rectNode["OffsetMin"].as<glm::vec2>();
                        if (rectNode["OffsetMax"]) cc.Transform.OffsetMax = rectNode["OffsetMax"].as<glm::vec2>();
                        if (rectNode["Pivot"]) cc.Transform.Pivot = rectNode["Pivot"].as<glm::vec2>();
                        if (rectNode["RectCoordinates"])
                            cc.Transform.RectCoordinates = rectNode["RectCoordinates"].as<glm::vec2>();
                    }

                    auto panelNode = entity["PanelControl"];
                    if (panelNode)
                    {
                        auto &pnl = deserializedEntity.AddComponent<PanelControl>();
                        if (panelNode["FullScreen"])
                            pnl.FullScreen = panelNode["FullScreen"].as<bool>();
                        if (panelNode["TexturePath"])
                            pnl.TexturePath = panelNode["TexturePath"].as<std::string>();

                        auto styleNode = panelNode["Style"];
                        if (styleNode)
                        {
                            if (styleNode["BackgroundColor"])
                                pnl.Style.BackgroundColor = styleNode["BackgroundColor"].as<Color>();
                            if (styleNode["Rounding"])
                                pnl.Style.Rounding = styleNode["Rounding"].as<float>();
                            if (styleNode["BorderSize"])
                                pnl.Style.BorderSize = styleNode["BorderSize"].as<float>();
                            if (styleNode["BorderColor"])
                                pnl.Style.BorderColor = styleNode["BorderColor"].as<Color>();
                        }
                    }

                    auto labelNode = entity["LabelControl"];
                    if (labelNode)
                    {
                        auto &lbl = deserializedEntity.AddComponent<LabelControl>();
                        if (labelNode["Text"])
                            lbl.Text = labelNode["Text"].as<std::string>();
                        if (labelNode["FontSize"])
                            lbl.Style.FontSize = labelNode["FontSize"].as<float>();
                        if (labelNode["TextColor"])
                            lbl.Style.TextColor = labelNode["TextColor"].as<Color>();
                        if (labelNode["Shadow"])
                            lbl.Style.Shadow = labelNode["Shadow"].as<bool>();
                        if (labelNode["ShadowOffset"])
                            lbl.Style.ShadowOffset = labelNode["ShadowOffset"].as<float>();
                        if (labelNode["ShadowColor"])
                            lbl.Style.ShadowColor = labelNode["ShadowColor"].as<Color>();
                        if (labelNode["LetterSpacing"])
                            lbl.Style.LetterSpacing = labelNode["LetterSpacing"].as<float>();
                        if (labelNode["Alignment"])
                            lbl.Style.Alignment = (TextAlignment)labelNode["Alignment"].as<int>();
                    }

                    auto buttonNode = entity["ButtonControl"];
                    if (buttonNode)
                    {
                        auto &btn = deserializedEntity.AddComponent<ButtonControl>();
                        if (buttonNode["Label"])
                            btn.Label = buttonNode["Label"].as<std::string>();
                        if (buttonNode["Interactable"])
                            btn.IsInteractable = buttonNode["Interactable"].as<bool>();

                        auto style = buttonNode["Style"];
                        if (style)
                        {
                            if (style["BackgroundColor"])
                                btn.Style.BackgroundColor = style["BackgroundColor"].as<Color>();
                            if (style["HoverColor"])
                                btn.Style.HoverColor = style["HoverColor"].as<Color>();
                            if (style["PressedColor"])
                                btn.Style.PressedColor = style["PressedColor"].as<Color>();
                        }

                        if (buttonNode["Action"])
                            btn.Action = (ButtonAction)buttonNode["Action"].as<int>();
                        if (buttonNode["TargetScene"])
                            btn.TargetScene = buttonNode["TargetScene"].as<std::string>();
                    }

                    auto sliderNode = entity["SliderControl"];
                    if (sliderNode)
                    {
                        auto &sl = deserializedEntity.AddComponent<SliderControl>();
                        if (sliderNode["Value"])
                            sl.Value = sliderNode["Value"].as<float>();
                        if (sliderNode["Min"])
                            sl.Min = sliderNode["Min"].as<float>();
                        if (sliderNode["Max"])
                            sl.Max = sliderNode["Max"].as<float>();
                    }

                    auto checkboxNode = entity["CheckboxControl"];
                    if (checkboxNode)
                    {
                        auto &cb = deserializedEntity.AddComponent<CheckboxControl>();
                        if (checkboxNode["Checked"])
                            cb.Checked = checkboxNode["Checked"].as<bool>();
                    }
                }

                auto playerComponent = entity["PlayerComponent"];
                if (playerComponent)
                {
                    auto &pc = deserializedEntity.AddComponent<PlayerComponent>();
                    if (playerComponent["MovementSpeed"])
                        pc.MovementSpeed = playerComponent["MovementSpeed"].as<float>();
                    if (playerComponent["LookSensitivity"])
                        pc.LookSensitivity = playerComponent["LookSensitivity"].as<float>();
                    if (playerComponent["CameraYaw"])
                        pc.CameraYaw = playerComponent["CameraYaw"].as<float>();
                    if (playerComponent["CameraPitch"])
                        pc.CameraPitch = playerComponent["CameraPitch"].as<float>();
                    if (playerComponent["CameraDistance"])
                        pc.CameraDistance = playerComponent["CameraDistance"].as<float>();
                    if (playerComponent["JumpForce"])
                        pc.JumpForce = playerComponent["JumpForce"].as<float>();
                }
            } // End of entity loop

            // Phase 3: Finalize Hierarchy
            for (auto &task : hierarchyTasks)
            {
                auto &hc = task.entity.AddComponent<HierarchyComponent>();
                if (task.parent != 0)
                {
                    Entity parent = m_Scene->GetEntityByUUID(task.parent);
                    if (parent)
                        hc.Parent = parent;
                }

                for (uint64_t childUUID : task.children)
                {
                    Entity child = m_Scene->GetEntityByUUID(childUUID);
                    if (child)
                        hc.Children.push_back(child);
                }
            }
        }

        return true;
    }
} // namespace CHEngine
