#include "scene_serializer.h"
#include "components.h"
#include "engine/core/log.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/render.h"
#include "scene.h"
#include "script_registry.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace YAML
{
template <> struct convert<Vector3>
{
    static Node encode(const Vector3 &rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        return node;
    }

    static bool decode(const Node &node, Vector3 &rhs)
    {
        if (node.IsSequence() && node.size() == 3)
        {
            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
        else if (node.IsMap())
        {
            rhs.x = node["x"] ? node["x"].as<float>() : 0.0f;
            rhs.y = node["y"] ? node["y"].as<float>() : 0.0f;
            rhs.z = node["z"] ? node["z"].as<float>() : 0.0f;
            return true;
        }
        return false;
    }
};

template <> struct convert<Color>
{
    static Node encode(const Color &rhs)
    {
        Node node;
        node.push_back(rhs.r);
        node.push_back(rhs.g);
        node.push_back(rhs.b);
        node.push_back(rhs.a);
        return node;
    }

    static bool decode(const Node &node, Color &rhs)
    {
        if (node.IsSequence() && node.size() == 4)
        {
            rhs.r = node[0].as<unsigned char>();
            rhs.g = node[1].as<unsigned char>();
            rhs.b = node[2].as<unsigned char>();
            rhs.a = node[3].as<unsigned char>();
            return true;
        }
        else if (node.IsMap())
        {
            rhs.r = node["r"] ? node["r"].as<unsigned char>() : 255;
            rhs.g = node["g"] ? node["g"].as<unsigned char>() : 255;
            rhs.b = node["b"] ? node["b"].as<unsigned char>() : 255;
            rhs.a = node["a"] ? node["a"].as<unsigned char>() : 255;
            return true;
        }
        return false;
    }
};
} // namespace YAML

namespace CHEngine
{
YAML::Emitter &operator<<(YAML::Emitter &out, const Vector3 &v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

YAML::Emitter &operator<<(YAML::Emitter &out, const Color &c)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << (int)c.r << (int)c.g << (int)c.b << (int)c.a << YAML::EndSeq;
    return out;
}

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
        out << YAML::Key << "Material";
        out << YAML::BeginMap;
        out << YAML::Key << "AlbedoColor" << YAML::Value << mc.Material.AlbedoColor;
        out << YAML::Key << "AlbedoPath" << YAML::Value << mc.Material.AlbedoPath;
        out << YAML::EndMap;
        out << YAML::EndMap;
    }

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

    out << YAML::EndMap; // Entity
}

bool SceneSerializer::Serialize(const std::string &filepath)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Scene" << YAML::Value << "Untitled";

    // Serialize Skybox
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

    std::ofstream fout(filepath);
    if (fout.is_open())
    {
        fout << out.c_str();
        CH_CORE_INFO("Scene saved successfully to: %s", filepath.c_str());
        return true;
    }
    else
    {
        CH_CORE_ERROR("Failed to save scene to: %s", filepath.c_str());
        return false;
    }
}

bool SceneSerializer::Deserialize(const std::string &filepath)
{
    std::ifstream stream(filepath);
    if (!stream.is_open())
    {
        CH_CORE_ERROR("Failed to open scene file: %s", filepath.c_str());
        return false;
    }

    std::stringstream strStream;
    strStream << stream.rdbuf();

    YAML::Node data = YAML::Load(strStream.str());
    if (!data["Scene"])
        return false;

    std::string sceneName = data["Scene"].as<std::string>();
    CH_CORE_INFO("Deserializing scene '%s'", sceneName.c_str());

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
        // Phase 1: Pre-collect and pre-load all assets asynchronously
        std::vector<std::string> modelPaths;
        for (auto entity : entities)
        {
            auto mc = entity["ModelComponent"];
            if (mc)
                modelPaths.push_back(mc["ModelPath"].as<std::string>());
            auto cc = entity["ColliderComponent"];
            if (cc && cc["Type"].as<int>() == (int)ColliderType::Mesh)
                modelPaths.push_back(cc["ModelPath"].as<std::string>());
        }

        // Trigger batch async load
        for (const auto &path : modelPaths)
        {
            if (!path.empty())
                AssetManager::LoadModelAsync(path);
        }

        struct HierarchyTask
        {
            Entity entity;
            uint64_t parent;
            std::vector<uint64_t> children;
        };
        std::vector<HierarchyTask> hierarchyTasks;

        struct BVHTask
        {
            Entity entity;
            std::string path;
            std::future<Ref<BVHNode>> future;
        };
        std::vector<BVHTask> bvhTasks;

        for (auto entity : entities)
        {
            uint64_t uuid = entity["Entity"].as<uint64_t>();

            std::string name;
            auto tagComponent = entity["TagComponent"];
            if (tagComponent)
                name = tagComponent["Tag"].as<std::string>();

            CH_CORE_TRACE("Deserialized entity with ID = %llu, name = %s", uuid, name.c_str());

            Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

            auto transformComponent = entity["TransformComponent"];
            if (transformComponent)
            {
                auto &tc = deserializedEntity.GetComponent<TransformComponent>();
                tc.Translation = transformComponent["Translation"].as<Vector3>();
                tc.Rotation = transformComponent["Rotation"].as<Vector3>();
                tc.Scale = transformComponent["Scale"].as<Vector3>();
            }

            auto modelComponent = entity["ModelComponent"];
            if (modelComponent)
            {
                auto &mc = deserializedEntity.AddComponent<ModelComponent>();
                mc.ModelPath = modelComponent["ModelPath"].as<std::string>();

                if (modelComponent["Material"])
                {
                    auto mat = modelComponent["Material"];
                    mc.Material.AlbedoColor = mat["AlbedoColor"].as<Color>();
                    mc.Material.AlbedoPath = mat["AlbedoPath"].as<std::string>();
                }
                else if (modelComponent["Tint"]) // Legacy support
                {
                    mc.Material.AlbedoColor = modelComponent["Tint"].as<Color>();
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
                // Legacy support for separate MaterialComponent
                if (!deserializedEntity.HasComponent<ModelComponent>())
                    deserializedEntity.AddComponent<ModelComponent>();

                auto &mc = deserializedEntity.GetComponent<ModelComponent>();
                mc.Material.AlbedoColor = materialComponent["AlbedoColor"].as<Color>();
                mc.Material.AlbedoPath = materialComponent["AlbedoPath"].as<std::string>();
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
                    // Block until model is loaded (it might be in cache or loading async)
                    Model model = AssetManager::LoadModel(cc.ModelPath);
                    if (model.meshCount > 0)
                    {
                        // Build BVH asynchronously
                        bvhTasks.push_back(
                            {deserializedEntity, cc.ModelPath, BVHBuilder::BuildAsync(model)});
                    }
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

        // Phase 4: Finalize all BVH tasks
        for (auto &task : bvhTasks)
        {
            if (task.entity.HasComponent<ColliderComponent>())
            {
                task.entity.GetComponent<ColliderComponent>().BVHRoot = task.future.get();
                CH_CORE_INFO("Parallel BVH Construction complete for: %s", task.path.c_str());
            }
        }
    }

    return true;
}
} // namespace CHEngine
