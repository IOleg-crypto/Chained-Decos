#include "scene_serializer.h"
#include "core/log.h"
#include "entity.h"
#include "scene/ecs/components/core/id_component.h"
#include "scene/ecs/components/core/tag_component.h"
#include "scene/ecs/components/physics_data.h"
#include "scene/ecs/components/player_component.h"
#include "scene/ecs/components/render_component.h"
#include "scene/ecs/components/scripting_components.h"
#include "scene/ecs/components/transform_component.h"
#include "scene/ecs/components/utility_components.h"
#include "scene/ecs/components/velocity_component.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace YAML
{

template <> struct convert<Vector2>
{
    static Node encode(const Vector2 &rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.SetStyle(EmitterStyle::Flow);
        return node;
    }

    static bool decode(const Node &node, Vector2 &rhs)
    {
        if (!node.IsSequence() || node.size() != 2)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        return true;
    }
};

template <> struct convert<Vector3>
{
    static Node encode(const Vector3 &rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        node.SetStyle(EmitterStyle::Flow);
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
        node.push_back((int)rhs.r);
        node.push_back((int)rhs.g);
        node.push_back((int)rhs.b);
        node.push_back((int)rhs.a);
        node.SetStyle(EmitterStyle::Flow);
        return node;
    }

    static bool decode(const Node &node, Color &rhs)
    {
        if (!node.IsSequence() || node.size() != 4)
            return false;

        rhs.r = (unsigned char)node[0].as<int>();
        rhs.g = (unsigned char)node[1].as<int>();
        rhs.b = (unsigned char)node[2].as<int>();
        rhs.a = (unsigned char)node[3].as<int>();
        return true;
    }
};

template <> struct convert<BoundingBox>
{
    static Node encode(const BoundingBox &rhs)
    {
        Node node;
        node["Min"] = rhs.min;
        node["Max"] = rhs.max;
        return node;
    }

    static bool decode(const Node &node, BoundingBox &rhs)
    {
        if (!node.IsMap())
            return false;

        rhs.min = node["Min"].as<Vector3>();
        rhs.max = node["Max"].as<Vector3>();
        return true;
    }
};

} // namespace YAML

namespace CHEngine
{

YAML::Emitter &operator<<(YAML::Emitter &out, const Vector2 &v)
{
    out << YAML::Flow << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
    return out;
}

YAML::Emitter &operator<<(YAML::Emitter &out, const Vector3 &v)
{
    out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

YAML::Emitter &operator<<(YAML::Emitter &out, const Color &c)
{
    out << YAML::Flow << YAML::BeginSeq << (int)c.r << (int)c.g << (int)c.b << (int)c.a
        << YAML::EndSeq;
    return out;
}

YAML::Emitter &operator<<(YAML::Emitter &out, const BoundingBox &b)
{
    out << YAML::BeginMap;
    out << YAML::Key << "Min" << YAML::Value << b.min;
    out << YAML::Key << "Max" << YAML::Value << b.max;
    out << YAML::EndMap;
    return out;
}

static void SerializeEntity(YAML::Emitter &out, Entity entity)
{
    out << YAML::BeginMap; // Entity
    out << YAML::Key << "Entity" << YAML::Value << entity.GetComponent<IDComponent>().ID;

    if (entity.HasComponent<TagComponent>())
    {
        out << YAML::Key << "TagComponent";
        out << YAML::BeginMap; // TagComponent
        out << YAML::Key << "Tag" << YAML::Value << entity.GetComponent<TagComponent>().Tag;
        out << YAML::EndMap; // TagComponent
    }

    if (entity.HasComponent<TransformComponent>())
    {
        out << YAML::Key << "TransformComponent";
        out << YAML::BeginMap; // TransformComponent
        auto &tc = entity.GetComponent<TransformComponent>();
        out << YAML::Key << "Translation" << YAML::Value << tc.position;
        out << YAML::Key << "Rotation" << YAML::Value << tc.rotation;
        out << YAML::Key << "Scale" << YAML::Value << tc.scale;
        out << YAML::EndMap; // TransformComponent
    }

    if (entity.HasComponent<RenderComponent>())
    {
        out << YAML::Key << "RenderComponent";
        out << YAML::BeginMap; // RenderComponent
        auto &rc = entity.GetComponent<RenderComponent>();
        out << YAML::Key << "ModelName" << YAML::Value << rc.modelName;
        out << YAML::Key << "Tint" << YAML::Value << rc.tint;
        out << YAML::Key << "Visible" << YAML::Value << rc.visible;
        out << YAML::Key << "RenderLayer" << YAML::Value << rc.renderLayer;
        out << YAML::Key << "Offset" << YAML::Value << rc.offset;
        out << YAML::EndMap; // RenderComponent
    }

    if (entity.HasComponent<CollisionComponent>())
    {
        out << YAML::Key << "CollisionComponent";
        out << YAML::BeginMap; // CollisionComponent
        auto &cc = entity.GetComponent<CollisionComponent>();
        out << YAML::Key << "Bounds" << YAML::Value << cc.bounds;
        out << YAML::Key << "IsTrigger" << YAML::Value << cc.isTrigger;
        out << YAML::Key << "CollisionLayer" << YAML::Value << cc.collisionLayer;
        out << YAML::Key << "CollisionMask" << YAML::Value << cc.collisionMask;
        out << YAML::EndMap; // CollisionComponent
    }

    if (entity.HasComponent<PhysicsData>())
    {
        out << YAML::Key << "PhysicsData";
        out << YAML::BeginMap; // PhysicsData
        auto &pd = entity.GetComponent<PhysicsData>();
        out << YAML::Key << "Mass" << YAML::Value << pd.mass;
        out << YAML::Key << "Gravity" << YAML::Value << pd.gravity;
        out << YAML::Key << "UseGravity" << YAML::Value << pd.useGravity;
        out << YAML::Key << "IsKinematic" << YAML::Value << pd.isKinematic;
        out << YAML::Key << "Friction" << YAML::Value << pd.friction;
        out << YAML::Key << "Bounciness" << YAML::Value << pd.bounciness;
        out << YAML::EndMap; // PhysicsData
    }

    if (entity.HasComponent<PlayerComponent>())
    {
        out << YAML::Key << "PlayerComponent";
        out << YAML::BeginMap; // PlayerComponent
        auto &pc = entity.GetComponent<PlayerComponent>();
        out << YAML::Key << "MoveSpeed" << YAML::Value << pc.moveSpeed;
        out << YAML::Key << "JumpForce" << YAML::Value << pc.jumpForce;
        out << YAML::Key << "MouseSensitivity" << YAML::Value << pc.mouseSensitivity;
        out << YAML::Key << "CameraPitch" << YAML::Value << pc.cameraPitch;
        out << YAML::Key << "CameraYaw" << YAML::Value << pc.cameraYaw;
        out << YAML::Key << "CameraDistance" << YAML::Value << pc.cameraDistance;
        out << YAML::Key << "SpawnPosition" << YAML::Value << pc.spawnPosition;
        out << YAML::EndMap; // PlayerComponent
    }

    if (entity.HasComponent<LifetimeComponent>())
    {
        out << YAML::Key << "LifetimeComponent";
        out << YAML::BeginMap; // LifetimeComponent
        auto &lc = entity.GetComponent<LifetimeComponent>();
        out << YAML::Key << "Lifetime" << YAML::Value << lc.lifetime;
        out << YAML::Key << "Timer" << YAML::Value << lc.timer;
        out << YAML::Key << "DestroyOnTimeout" << YAML::Value << lc.destroyOnTimeout;
        out << YAML::EndMap; // LifetimeComponent
    }

    if (entity.HasComponent<CSharpScriptComponent>())
    {
        out << YAML::Key << "CSharpScriptComponent";
        out << YAML::BeginMap; // CSharpScriptComponent
        auto &sc = entity.GetComponent<CSharpScriptComponent>();
        out << YAML::Key << "ClassName" << YAML::Value << sc.className;
        out << YAML::EndMap; // CSharpScriptComponent
    }

    if (entity.HasComponent<MapObjectIndex>())
    {
        out << YAML::Key << "MapObjectIndex";
        out << YAML::BeginMap;
        out << YAML::Key << "Index" << YAML::Value << entity.GetComponent<MapObjectIndex>().index;
        out << YAML::EndMap;
    }

    out << YAML::EndMap; // Entity
}

ECSSceneSerializer::ECSSceneSerializer(const std::shared_ptr<Scene> &scene) : m_Scene(scene)
{
}

void ECSSceneSerializer::Serialize(const std::string &filepath)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetName();
    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

    for (auto [entityID] : m_Scene->m_Registry.storage<entt::entity>().each())
    {
        Entity entity = {entityID, m_Scene.get()};
        if (!entity)
            continue;

        SerializeEntity(out, entity);
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream fout(filepath);
    fout << out.c_str();

    CD_CORE_INFO("Scene serialized to %s", filepath.c_str());
}

bool ECSSceneSerializer::Deserialize(const std::string &filepath)
{
    YAML::Node data;
    try
    {
        data = YAML::LoadFile(filepath);
    }
    catch (YAML::ParserException e)
    {
        CD_CORE_ERROR("Failed to load .chscene file '%s'\n     %s", filepath.c_str(), e.what());
        return false;
    }

    if (!data["Scene"])
        return false;

    std::string sceneName = data["Scene"].as<std::string>();
    m_Scene->SetName(sceneName);
    CD_CORE_TRACE("Deserializing scene '%s'", sceneName.c_str());

    auto entities = data["Entities"];
    if (entities)
    {
        for (auto entity : entities)
        {
            uint64_t uuid = entity["Entity"].as<uint64_t>();

            std::string name;
            auto tagComponent = entity["TagComponent"];
            if (tagComponent)
                name = tagComponent["Tag"].as<std::string>();

            Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

            auto transformComponent = entity["TransformComponent"];
            if (transformComponent)
            {
                auto &tc = deserializedEntity.GetComponent<TransformComponent>();
                tc.position = transformComponent["Translation"].as<Vector3>();
                tc.rotation = transformComponent["Rotation"].as<Vector3>();
                tc.scale = transformComponent["Scale"].as<Vector3>();
            }

            auto renderComponent = entity["RenderComponent"];
            if (renderComponent)
            {
                auto &rc = deserializedEntity.AddComponent<RenderComponent>();
                rc.modelName = renderComponent["ModelName"].as<std::string>();
                rc.tint = renderComponent["Tint"].as<Color>();
                rc.visible = renderComponent["Visible"].as<bool>();
                rc.renderLayer = renderComponent["RenderLayer"].as<int>();
                rc.offset = renderComponent["Offset"].as<Vector3>();
            }

            auto collisionComponent = entity["CollisionComponent"];
            if (collisionComponent)
            {
                auto &cc = deserializedEntity.AddComponent<CollisionComponent>();
                if (collisionComponent["Bounds"])
                    cc.bounds = collisionComponent["Bounds"].as<BoundingBox>();
                cc.isTrigger = collisionComponent["IsTrigger"].as<bool>();
                cc.collisionLayer = collisionComponent["CollisionLayer"].as<int>();
                cc.collisionMask = collisionComponent["CollisionMask"].as<int>();
            }

            auto physicsData = entity["PhysicsData"];
            if (physicsData)
            {
                auto &pd = deserializedEntity.AddComponent<PhysicsData>();
                pd.mass = physicsData["Mass"].as<float>();
                pd.gravity = physicsData["Gravity"].as<float>();
                pd.useGravity = physicsData["UseGravity"].as<bool>();
                pd.isKinematic = physicsData["IsKinematic"].as<bool>();
                pd.friction = physicsData["Friction"].as<float>();
                pd.bounciness = physicsData["Bounciness"].as<float>();
            }

            auto playerComponent = entity["PlayerComponent"];
            if (playerComponent)
            {
                auto &pc = deserializedEntity.AddComponent<PlayerComponent>();
                pc.moveSpeed = playerComponent["MoveSpeed"].as<float>();
                pc.jumpForce = playerComponent["JumpForce"].as<float>();
                pc.mouseSensitivity = playerComponent["MouseSensitivity"].as<float>();
                pc.cameraPitch = playerComponent["CameraPitch"].as<float>();
                pc.cameraYaw = playerComponent["CameraYaw"].as<float>();
                pc.cameraDistance = playerComponent["CameraDistance"].as<float>();
                pc.spawnPosition = playerComponent["SpawnPosition"].as<Vector3>();

                if (!deserializedEntity.HasComponent<VelocityComponent>())
                    deserializedEntity.AddComponent<VelocityComponent>();
            }

            auto lifetimeComponent = entity["LifetimeComponent"];
            if (lifetimeComponent)
            {
                auto &lc = deserializedEntity.AddComponent<LifetimeComponent>();
                lc.lifetime = lifetimeComponent["Lifetime"].as<float>();
                lc.timer = lifetimeComponent["Timer"].as<float>();
                lc.destroyOnTimeout = lifetimeComponent["DestroyOnTimeout"].as<bool>();
            }

            auto csharpScriptComponent = entity["CSharpScriptComponent"];
            if (csharpScriptComponent)
            {
                auto &sc = deserializedEntity.AddComponent<CSharpScriptComponent>();
                sc.className = csharpScriptComponent["ClassName"].as<std::string>();
            }

            auto mapObjectIndex = entity["MapObjectIndex"];
            if (mapObjectIndex)
            {
                auto &moi = deserializedEntity.AddComponent<MapObjectIndex>();
                moi.index = mapObjectIndex["Index"].as<int>();
            }
        }
    }

    return true;
}

} // namespace CHEngine
