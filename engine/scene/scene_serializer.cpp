#include "scene_serializer.h"
#include "components.h"
#include "engine/core/log.h"
#include "engine/core/yaml_utils.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/render/asset_manager.h"
#include "engine/render/render.h"
#include "scene.h"
#include "script_registry.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

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
        out << YAML::Key << "Entity" << YAML::Value
            << (uint64_t)entity.GetComponent<IDComponent>().ID;
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
                out << YAML::Key << "MetallicRoughnessPath" << YAML::Value
                    << slot.Material.MetallicRoughnessPath;
                out << YAML::Key << "OverrideMetallicRoughness" << YAML::Value
                    << slot.Material.OverrideMetallicRoughness;
                out << YAML::Key << "EmissivePath" << YAML::Value << slot.Material.EmissivePath;
                out << YAML::Key << "OverrideEmissive" << YAML::Value
                    << slot.Material.OverrideEmissive;
                out << YAML::Key << "Metalness" << YAML::Value << slot.Material.Metalness;
                out << YAML::Key << "Roughness" << YAML::Value << slot.Material.Roughness;
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

    // MaterialComponent is now deprecated and merged into ModelComponent
    // if (entity.HasComponent<MaterialComponent>()) ...

    if (entity.HasComponent<ColliderComponent>())
    {
        out << YAML::Key << "ColliderComponent";
        auto &cc = entity.GetComponent<ColliderComponent>();
        out << YAML::BeginMap;
        out << YAML::Key << "Type" << YAML::Value << (int)cc.Type;
        out << YAML::Key << "bEnabled" << YAML::Value << cc.bEnabled;
        out << YAML::Key << "Offset" << YAML::Value << cc.Offset;
        out << YAML::Key << "Size" << YAML::Value << cc.Size;
        out << YAML::Key << "bAutoCalculate" << YAML::Value << cc.bAutoCalculate;
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
        out << YAML::Key << "CurrentAnimationIndex" << YAML::Value
            << animation.CurrentAnimationIndex;
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
            out << YAML::Key << "Parent" << YAML::Value
                << (uint64_t)parent.GetComponent<IDComponent>().ID;
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

    if (entity.HasComponent<WidgetComponent>())
    {
        out << YAML::Key << "WidgetComponent";
        auto &ui = entity.GetComponent<WidgetComponent>();
        out << YAML::BeginMap;
        out << YAML::Key << "IsActive" << YAML::Value << ui.IsActive;
        out << YAML::Key << "HiddenInHierarchy" << YAML::Value << ui.HiddenInHierarchy;

        out << YAML::Key << "RectTransform" << YAML::BeginMap;
        out << YAML::Key << "AnchorMin" << YAML::Value << ui.Transform.AnchorMin;
        out << YAML::Key << "AnchorMax" << YAML::Value << ui.Transform.AnchorMax;
        out << YAML::Key << "OffsetMin" << YAML::Value << ui.Transform.OffsetMin;
        out << YAML::Key << "OffsetMax" << YAML::Value << ui.Transform.OffsetMax;
        out << YAML::Key << "Pivot" << YAML::Value << ui.Transform.Pivot;
        out << YAML::Key << "RectCoordinates" << YAML::Value << ui.Transform.RectCoordinates;
        out << YAML::EndMap;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<PanelWidget>())
    {
        out << YAML::Key << "PanelWidget";
        auto &panel = entity.GetComponent<PanelWidget>();
        out << YAML::BeginMap;
        out << YAML::Key << "FullScreen" << YAML::Value << panel.FullScreen;
        out << YAML::Key << "TexturePath" << YAML::Value << panel.TexturePath;

        out << YAML::Key << "Style" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "BackgroundColor" << YAML::Value << panel.Style.BackgroundColor;
        out << YAML::Key << "Rounding" << YAML::Value << panel.Style.Rounding;
        out << YAML::Key << "BorderSize" << YAML::Value << panel.Style.BorderSize;
        out << YAML::Key << "BorderColor" << YAML::Value << panel.Style.BorderColor;
        out << YAML::EndMap;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<LabelWidget>())
    {
        out << YAML::Key << "LabelWidget";
        auto &lbl = entity.GetComponent<LabelWidget>();
        out << YAML::BeginMap;
        out << YAML::Key << "Text" << YAML::Value << lbl.Text;
        out << YAML::Key << "FontSize" << YAML::Value << lbl.Style.FontSize;
        out << YAML::Key << "TextColor" << YAML::Value << lbl.Style.TextColor;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<ButtonWidget>())
    {
        out << YAML::Key << "ButtonWidget";
        auto &btn = entity.GetComponent<ButtonWidget>();
        out << YAML::BeginMap;
        out << YAML::Key << "Label" << YAML::Value << btn.Label;
        out << YAML::Key << "Interactable" << YAML::Value << btn.IsInteractable;

        out << YAML::Key << "Style" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "BackgroundColor" << YAML::Value << btn.Style.BackgroundColor;
        out << YAML::Key << "HoverColor" << YAML::Value << btn.Style.HoverColor;
        out << YAML::Key << "PressedColor" << YAML::Value << btn.Style.PressedColor;
        out << YAML::EndMap;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<SliderWidget>())
    {
        out << YAML::Key << "SliderWidget";
        auto &sl = entity.GetComponent<SliderWidget>();
        out << YAML::BeginMap;
        out << YAML::Key << "Value" << YAML::Value << sl.Value;
        out << YAML::Key << "Min" << YAML::Value << sl.Min;
        out << YAML::Key << "Max" << YAML::Value << sl.Max;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<CheckboxWidget>())
    {
        out << YAML::Key << "CheckboxWidget";
        auto &cb = entity.GetComponent<CheckboxWidget>();
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
    if (m_Scene->m_Environment)
    {
        out << YAML::Key << "EnvironmentPath" << YAML::Value << m_Scene->m_Environment->GetPath();
    }

    // Keep legacy Skybox for now to avoid breaking existing scenes
    {
        auto &sc = m_Scene->GetSkybox();
        out << YAML::Key << "Skybox";
        out << YAML::BeginMap;
        out << YAML::Key << "TexturePath" << YAML::Value << sc.TexturePath;
        out << YAML::Key << "Exposure" << YAML::Value << sc.Exposure;
        out << YAML::Key << "Brightness" << YAML::Value << sc.Brightness;
        out << YAML::Key << "Contrast" << YAML::Value << sc.Contrast;
        out << YAML::EndMap;
    }

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

    auto background = data["Background"];
    if (background)
    {
        if (background["Mode"])
            m_Scene->SetBackgroundMode((BackgroundMode)background["Mode"].as<int>());
        if (background["Color"])
            m_Scene->SetBackgroundColor(background["Color"].as<Color>());
        if (background["TexturePath"])
            m_Scene->SetBackgroundTexturePath(background["TexturePath"].as<std::string>());
    }

    // Deserialize Environment
    auto envPath = data["EnvironmentPath"];
    if (envPath)
    {
        m_Scene->m_Environment = AssetManager::LoadEnvironment(envPath.as<std::string>());
    }

    // Deserialize Skybox
    auto skybox = data["Skybox"];
    if (skybox)
    {
        auto &sc = m_Scene->GetSkybox();
        sc.TexturePath = skybox["TexturePath"].as<std::string>();
        sc.Exposure = skybox["Exposure"].as<float>();
        sc.Brightness = skybox["Brightness"].as<float>();
        sc.Contrast = skybox["Contrast"].as<float>();
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
                CH_CORE_WARN("SceneSerializer: Duplicate UUID {0} found! Regenerated as {1}",
                             oldUUID, uuid);
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
                            slot.Material.MetallicRoughnessPath =
                                mat["MetallicRoughnessPath"].as<std::string>();
                        if (mat["OverrideMetallicRoughness"])
                            slot.Material.OverrideMetallicRoughness =
                                mat["OverrideMetallicRoughness"].as<bool>();
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
                sc.IsActive = spawnComponent["IsActive"].as<bool>();
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
                            slot.Material.MetallicRoughnessPath =
                                mat["MetallicRoughnessPath"].as<std::string>();
                        if (mat["OverrideMetallicRoughness"])
                            slot.Material.OverrideMetallicRoughness =
                                mat["OverrideMetallicRoughness"].as<bool>();
                        if (mat["EmissivePath"])
                            slot.Material.EmissivePath = mat["EmissivePath"].as<std::string>();
                        if (mat["OverrideEmissive"])
                            slot.Material.OverrideEmissive = mat["OverrideEmissive"].as<bool>();
                        if (mat["Metalness"])
                            slot.Material.Metalness = mat["Metalness"].as<float>();
                        if (mat["Roughness"])
                            slot.Material.Roughness = mat["Roughness"].as<float>();

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
                cc.Type = (ColliderType)colliderComponent["Type"].as<int>();
                cc.bEnabled = colliderComponent["bEnabled"].as<bool>();
                cc.Offset = colliderComponent["Offset"].as<Vector3>();
                cc.Size = colliderComponent["Size"].as<Vector3>();
                cc.bAutoCalculate = colliderComponent["bAutoCalculate"].as<bool>();
                cc.ModelPath = colliderComponent["ModelPath"].as<std::string>();

                if (cc.Type == ColliderType::Mesh && !cc.ModelPath.empty())
                {
                    auto asset = AssetManager::Get<ModelAsset>(cc.ModelPath);
                    if (asset)
                        cc.BVHRoot = asset->GetBVHCache();
                }
            }

            auto audioComponent = entity["AudioComponent"];
            if (audioComponent)
            {
                auto &ac = deserializedEntity.AddComponent<AudioComponent>();
                ac.SoundPath = audioComponent["SoundPath"].as<std::string>();
                ac.Volume = audioComponent["Volume"].as<float>();
                ac.Pitch = audioComponent["Pitch"].as<float>();
                ac.Loop = audioComponent["Loop"].as<bool>();
                ac.PlayOnStart = audioComponent["PlayOnStart"].as<bool>();
            }

            auto cameraComponent = entity["CameraComponent"];
            if (cameraComponent)
            {
                auto &cc = deserializedEntity.AddComponent<CameraComponent>();
                cc.Fov = cameraComponent["Fov"].as<float>();
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
                anim.AnimationPath = animationComponent["AnimationPath"].as<std::string>();
                anim.CurrentAnimationIndex = animationComponent["CurrentAnimationIndex"].as<int>();
                anim.IsLooping = animationComponent["IsLooping"].as<bool>();
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
                rb.Velocity = rigidBodyComponent["Velocity"].as<Vector3>();
                rb.UseGravity = rigidBodyComponent["UseGravity"].as<bool>();
                rb.IsKinematic = rigidBodyComponent["IsKinematic"].as<bool>();
                rb.Mass = rigidBodyComponent["Mass"].as<float>();
            }

            auto widgetNode = entity["WidgetComponent"];
            if (widgetNode)
            {
                auto &ui = deserializedEntity.AddComponent<WidgetComponent>();
                if (widgetNode["IsActive"])
                    ui.IsActive = widgetNode["IsActive"].as<bool>();
                if (widgetNode["HiddenInHierarchy"])
                    ui.HiddenInHierarchy = widgetNode["HiddenInHierarchy"].as<bool>();

                auto rectNode = widgetNode["RectTransform"];
                if (rectNode)
                {
                    ui.Transform.AnchorMin = rectNode["AnchorMin"].as<Vector2>();
                    ui.Transform.AnchorMax = rectNode["AnchorMax"].as<Vector2>();
                    ui.Transform.OffsetMin = rectNode["OffsetMin"].as<Vector2>();
                    ui.Transform.OffsetMax = rectNode["OffsetMax"].as<Vector2>();
                    ui.Transform.Pivot = rectNode["Pivot"].as<Vector2>();
                    if (rectNode["RectCoordinates"])
                        ui.Transform.RectCoordinates = rectNode["RectCoordinates"].as<Vector2>();
                }

                auto panelNode = entity["PanelWidget"];
                if (panelNode)
                {
                    auto &pnl = deserializedEntity.AddComponent<PanelWidget>();
                    if (panelNode["FullScreen"])
                        pnl.FullScreen = panelNode["FullScreen"].as<bool>();
                    if (panelNode["TexturePath"])
                        pnl.TexturePath = panelNode["TexturePath"].as<std::string>();

                    auto style = panelNode["Style"];
                    if (style)
                    {
                        if (style["BackgroundColor"])
                            pnl.Style.BackgroundColor = style["BackgroundColor"].as<Color>();
                        if (style["Rounding"])
                            pnl.Style.Rounding = style["Rounding"].as<float>();
                        if (style["BorderSize"])
                            pnl.Style.BorderSize = style["BorderSize"].as<float>();
                        if (style["BorderColor"])
                            pnl.Style.BorderColor = style["BorderColor"].as<Color>();
                    }
                }

                auto labelNode = entity["LabelWidget"];
                if (labelNode)
                {
                    auto &lbl = deserializedEntity.AddComponent<LabelWidget>();
                    if (labelNode["Text"])
                        lbl.Text = labelNode["Text"].as<std::string>();
                    if (labelNode["FontSize"])
                        lbl.Style.FontSize = labelNode["FontSize"].as<float>();
                    if (labelNode["TextColor"])
                        lbl.Style.TextColor = labelNode["TextColor"].as<Color>();
                }

                auto buttonNode = entity["ButtonWidget"];
                if (buttonNode)
                {
                    auto &btn = deserializedEntity.AddComponent<ButtonWidget>();
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
                }

                auto sliderNode = entity["SliderWidget"];
                if (sliderNode)
                {
                    auto &sl = deserializedEntity.AddComponent<SliderWidget>();
                    if (sliderNode["Value"])
                        sl.Value = sliderNode["Value"].as<float>();
                    if (sliderNode["Min"])
                        sl.Min = sliderNode["Min"].as<float>();
                    if (sliderNode["Max"])
                        sl.Max = sliderNode["Max"].as<float>();
                }

                auto checkboxNode = entity["CheckboxWidget"];
                if (checkboxNode)
                {
                    auto &cb = deserializedEntity.AddComponent<CheckboxWidget>();
                    if (checkboxNode["Checked"])
                        cb.Checked = checkboxNode["Checked"].as<bool>();
                }

                // --- LEGACY CLEANUP --- (Omitted old migration logic to avoid bloat)
            }

            // Legacy placeholders removed. Unified widgets are processed above.

            auto playerComponent = entity["PlayerComponent"];
            if (playerComponent)
            {
                auto &pc = deserializedEntity.AddComponent<PlayerComponent>();
                pc.MovementSpeed = playerComponent["MovementSpeed"].as<float>();
                pc.LookSensitivity = playerComponent["LookSensitivity"].as<float>();
                pc.CameraYaw = playerComponent["CameraYaw"].as<float>();
                pc.CameraPitch = playerComponent["CameraPitch"].as<float>();
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
