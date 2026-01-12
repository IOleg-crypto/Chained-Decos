#include "scene_serializer.h"
#include "components.h"
#include "engine/core/log.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/render.h"
#include "scene.h"

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
        if (!node.IsSequence() || node.size() != 3)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        rhs.z = node[2].as<float>();
        return true;
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
        if (!node.IsSequence() || node.size() != 4)
            return false;

        rhs.r = node[0].as<unsigned char>();
        rhs.g = node[1].as<unsigned char>();
        rhs.b = node[2].as<unsigned char>();
        rhs.a = node[3].as<unsigned char>();
        return true;
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
    out << YAML::BeginMap;                                             // Entity
    out << YAML::Key << "Entity" << YAML::Value << "1283719283719283"; // TODO: UUID

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
        out << YAML::Key << "Tint" << YAML::Value << mc.Tint;
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

    if (entity.HasComponent<MaterialComponent>())
    {
        out << YAML::Key << "MaterialComponent";
        out << YAML::BeginMap;
        out << YAML::Key << "AlbedoColor" << YAML::Value
            << entity.GetComponent<MaterialComponent>().AlbedoColor;
        out << YAML::Key << "AlbedoPath" << YAML::Value
            << entity.GetComponent<MaterialComponent>().AlbedoPath;
        out << YAML::EndMap;
    }

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

    if (entity.HasComponent<CSharpScriptComponent>())
    {
        out << YAML::Key << "CSharpScriptComponent";
        auto &sc = entity.GetComponent<CSharpScriptComponent>();
        out << YAML::BeginMap;
        out << YAML::Key << "ClassName" << YAML::Value << sc.ClassName;
        out << YAML::EndMap;
    }

    out << YAML::EndMap; // Entity
}

void SceneSerializer::Serialize(const std::string &filepath)
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

    for (auto entityID : m_Scene->GetRegistry().storage<entt::entity>())
    {
        Entity entity = {entityID, m_Scene};
        SerializeEntity(out, entity);
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream fout(filepath);
    if (fout.is_open())
    {
        fout << out.c_str();
        CH_CORE_INFO("Scene saved successfully to: %s", filepath.c_str());
    }
    else
    {
        CH_CORE_ERROR("Failed to save scene to: %s", filepath.c_str());
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

        // Phase 2: Actual deserialization with BVH generation dispatch
        struct BVHTask
        {
            ColliderComponent *cc;
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

            Entity deserializedEntity = m_Scene->CreateEntity(name);

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
                mc.Tint = modelComponent["Tint"].as<Color>();
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
                auto &mc = deserializedEntity.AddComponent<MaterialComponent>();
                mc.AlbedoColor = materialComponent["AlbedoColor"].as<Color>();
                mc.AlbedoPath = materialComponent["AlbedoPath"].as<std::string>();
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
                        bvhTasks.push_back({&cc, cc.ModelPath, BVHBuilder::BuildAsync(model)});
                    }
                }
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
        }

        // Finalize all BVH tasks
        for (auto &task : bvhTasks)
        {
            task.cc->BVHRoot = task.future.get();
            CH_CORE_INFO("Parallel BVH Construction complete for: %s", task.path.c_str());
        }
    }

    return true;
}
} // namespace CHEngine
