#include "scene_serializer.h"
#include "component_serializer.h"
#include "hierarchy_serializer.h"
#include "components.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/log.h"
#include "engine/scene/yaml.h"
#include "engine/graphics/assets/environment.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/scene/project.h"
#include "scene.h"
#include "yaml-cpp/yaml.h"
#include <fstream>

namespace CHEngine
{
namespace
{
template <typename T>
T ReadYamlValue(const YAML::Node& node, const char* key, const T& fallback)
{
    if (!node || !node[key])
    {
        return fallback;
    }

    return node[key].as<T>(fallback);
}
} // namespace

static void SerializeEntity(YAML::Emitter& out, Entity entity)
{
    out << YAML::BeginMap; // Entity

    ComponentSerializer::Get().SerializeID(out, entity);
    ComponentSerializer::Get().SerializeAll(out, entity);

    out << YAML::EndMap; // Entity
}

std::string SceneSerializer::SerializeToString()
{
    if (!m_Scene)
    {
        return "";
    }

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetSettings().Name;

    // Serialize Background Settings
    out << YAML::Key << "Background" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Mode" << YAML::Value << (int)m_Scene->GetSettings().Mode;
    out << YAML::Key << "Color" << YAML::Value << m_Scene->GetSettings().BackgroundColor;
    out << YAML::Key << "TexturePath" << YAML::Value
        << Project::GetRelativePath(m_Scene->GetSettings().BackgroundTexturePath);
    out << YAML::EndMap;

    // Serialize Canvas Settings
    out << YAML::Key << "Canvas" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "ReferenceResolution" << YAML::Value << m_Scene->GetSettings().Canvas.ReferenceResolution;
    out << YAML::Key << "ScaleMode" << YAML::Value << (int)m_Scene->GetSettings().Canvas.ScaleMode;
    out << YAML::Key << "MatchWidthOrHeight" << YAML::Value << m_Scene->GetSettings().Canvas.MatchWidthOrHeight;
    out << YAML::EndMap;

    // Serialize Environment
    if (m_Scene->GetSettings().Environment)
    {
        out << YAML::Key << "EnvironmentPath" << YAML::Value
            << Project::GetRelativePath(m_Scene->GetSettings().Environment->GetPath());

        // Also serialize the current settings for quick preview/fallback
        auto& settings = m_Scene->GetSettings().Environment->GetSettings();

        out << YAML::Key << "Lighting" << YAML::BeginMap;
        out << YAML::Key << "Direction" << YAML::Value << settings.Lighting.Direction;
        out << YAML::Key << "LightColor" << YAML::Value << settings.Lighting.LightColor;
        out << YAML::Key << "Ambient" << YAML::Value << settings.Lighting.Ambient;
        out << YAML::EndMap;

        out << YAML::Key << "Skybox" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "TexturePath" << YAML::Value << Project::GetRelativePath(settings.Skybox.TexturePath);
        out << YAML::Key << "Mode" << YAML::Value << settings.Skybox.Mode;
        out << YAML::Key << "Exposure" << YAML::Value << settings.Skybox.Exposure;
        out << YAML::Key << "Brightness" << YAML::Value << settings.Skybox.Brightness;
        out << YAML::Key << "Contrast" << YAML::Value << settings.Skybox.Contrast;
        out << YAML::EndMap;

        out << YAML::Key << "Fog" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Enabled" << YAML::Value << settings.Fog.Enabled;
        out << YAML::Key << "Color" << YAML::Value << settings.Fog.FogColor;
        out << YAML::Key << "Density" << YAML::Value << settings.Fog.Density;
        out << YAML::Key << "Start" << YAML::Value << settings.Fog.Start;
        out << YAML::Key << "End" << YAML::Value << settings.Fog.End;
        out << YAML::EndMap;
    }

    out << YAML::Key << "DebugSettings" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "DiagnosticMode" << YAML::Value << m_Scene->GetSettings().DiagnosticMode;
    out << YAML::Key << "DrawColliders" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawColliders;
    out << YAML::Key << "DrawHierarchy" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawHierarchy;
    out << YAML::Key << "DrawCollisionModelBox" << YAML::Value
        << m_Scene->GetSettings().DebugFlags.DrawCollisionModelBox;
    out << YAML::Key << "DrawGrid" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawGrid;
    out << YAML::Key << "DrawSelection" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawSelection;
    out << YAML::Key << "DrawLights" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawLights;
    out << YAML::Key << "DrawSpawnZones" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawSpawnZones;
    out << YAML::Key << "CollisionWireframeMode" << YAML::Value << m_Scene->GetSettings().DebugFlags.SetCollisionWireframeMode;
    out << YAML::EndMap;

    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

    m_Scene->GetRegistry().view<IDComponent>().each([&](auto entityID, auto& id) {
        Entity entity(entityID, &m_Scene->GetRegistry());
        SerializeEntity(out, entity);
    });

    out << YAML::EndSeq;
    out << YAML::EndMap;

    return std::string(out.c_str());
}

bool SceneSerializer::Serialize(const std::string& filepath)
{
    std::string yaml = SerializeToString();
    std::ofstream fout(filepath);
    if (fout.is_open())
    {
        fout << yaml;
        return true;
    }
    else
    {
        return false;
    }
}

SceneSerializer::SceneSerializer(Scene* scene)
    : m_Scene(scene)
{
}

bool SceneSerializer::Deserialize(const std::string& filepath)
{
    m_LastError.clear();

    std::ifstream stream(filepath);
    if (!stream.is_open())
    {
        m_LastError = "SceneSerializer: failed to open scene file '" + filepath + "'";
        return false;
    }

    std::stringstream strStream;
    strStream << stream.rdbuf();

    return DeserializeFromString(strStream.str());
}

bool SceneSerializer::DeserializeFromString(const std::string& yaml)
{
    m_LastError.clear();

    YAML::Node data;
    try
    {
        data = YAML::Load(yaml);
    }
    catch (const std::exception& e)
    {
        m_LastError = std::string("SceneSerializer: invalid YAML: ") + e.what();
        return false;
    }
    catch (...)
    {
        m_LastError = "SceneSerializer: invalid YAML with an unknown exception";
        return false;
    }

    if (!data["Scene"] || !data["Scene"].IsScalar())
    {
        m_LastError = "SceneSerializer: missing Scene root key";
        return false;
    }

    m_Scene->GetSettings().Name = ReadYamlValue(data, "Scene", m_Scene->GetSettings().Name);

    // Deserialize Background
    if (data["Background"])
    {
        auto background = data["Background"];
        m_Scene->GetSettings().Mode = static_cast<BackgroundMode>(
            ReadYamlValue(background, "Mode", static_cast<int>(m_Scene->GetSettings().Mode)));
        m_Scene->GetSettings().BackgroundColor =
            ReadYamlValue(background, "Color", m_Scene->GetSettings().BackgroundColor);
        if (background["TexturePath"] && background["TexturePath"].IsScalar())
        {
            m_Scene->GetSettings().BackgroundTexturePath =
                ReadYamlValue(background, "TexturePath", m_Scene->GetSettings().BackgroundTexturePath);
        }
        // Legacy AmbientIntensity in Background block is silently ignored
    }

    // Deserialize Canvas
    if (data["Canvas"])
    {
        auto canvas = data["Canvas"];
        m_Scene->GetSettings().Canvas.ReferenceResolution =
            ReadYamlValue(canvas, "ReferenceResolution", m_Scene->GetSettings().Canvas.ReferenceResolution);
        m_Scene->GetSettings().Canvas.ScaleMode = static_cast<CanvasScaleMode>(
            ReadYamlValue(canvas, "ScaleMode", static_cast<int>(m_Scene->GetSettings().Canvas.ScaleMode)));
        m_Scene->GetSettings().Canvas.MatchWidthOrHeight =
            ReadYamlValue(canvas, "MatchWidthOrHeight", m_Scene->GetSettings().Canvas.MatchWidthOrHeight);
    }

    // Deserialize Debug Settings
    if (data["DebugSettings"])
    {
        auto debugNode = data["DebugSettings"];
        m_Scene->GetSettings().DiagnosticMode = ReadYamlValue(debugNode, "DiagnosticMode", 0.0f);
        m_Scene->GetSettings().DebugFlags.DrawColliders = ReadYamlValue(debugNode, "DrawColliders", false);
        m_Scene->GetSettings().DebugFlags.DrawHierarchy = ReadYamlValue(debugNode, "DrawHierarchy", false);
        m_Scene->GetSettings().DebugFlags.DrawCollisionModelBox =
            ReadYamlValue(debugNode, "DrawCollisionModelBox", false);
        m_Scene->GetSettings().DebugFlags.DrawGrid = ReadYamlValue(debugNode, "DrawGrid", false);
        m_Scene->GetSettings().DebugFlags.DrawSelection = ReadYamlValue(debugNode, "DrawSelection", true);
        m_Scene->GetSettings().DebugFlags.DrawLights = ReadYamlValue(debugNode, "DrawLights", true);
        m_Scene->GetSettings().DebugFlags.DrawSpawnZones = ReadYamlValue(debugNode, "DrawSpawnZones", true);
        m_Scene->GetSettings().DebugFlags.SetCollisionWireframeMode =
            ReadYamlValue(debugNode, "CollisionWireframeMode", 0);
    }

    // Deserialize Environment
    if (data["EnvironmentPath"] && data["EnvironmentPath"].IsScalar())
    {
        std::string envPath = ReadYamlValue(data, "EnvironmentPath", std::string());
        if (auto project = Project::GetActive())
        {
            m_Scene->GetSettings().Environment = AssetManager::Get().Get<EnvironmentAsset>(envPath);
        }
    }

    // Deserialize Environment Settings (Skybox + Fog + Lighting)
    if (data["Skybox"] || data["Fog"] || data["LightDirection"])
    {
        // Ensure Environment exists
        if (!m_Scene->GetSettings().Environment)
        {
            m_Scene->GetSettings().Environment = std::make_shared<EnvironmentAsset>();
        }

        auto env = m_Scene->GetSettings().Environment;
        auto& settings = env->GetSettings();

        // Lighting (new format with Lighting section, or backward-compat flat fields)
        if (data["Lighting"])
        {
            auto lighting = data["Lighting"];
            settings.Lighting.Direction = ReadYamlValue(lighting, "Direction", settings.Lighting.Direction);
            settings.Lighting.LightColor = ReadYamlValue(lighting, "LightColor", settings.Lighting.LightColor);
            settings.Lighting.Ambient = ReadYamlValue(lighting, "Ambient", settings.Lighting.Ambient);
        }
        else
        {
            // Backward compat: old flat field names
            settings.Lighting.Direction = ReadYamlValue(data, "LightDirection", settings.Lighting.Direction);
            settings.Lighting.LightColor = ReadYamlValue(data, "LightColor", settings.Lighting.LightColor);
            settings.Lighting.Ambient = ReadYamlValue(data, "AmbientIntensity", settings.Lighting.Ambient);
        }

        // Skybox
        if (auto skybox = data["Skybox"])
        {
            if (skybox["TexturePath"] && skybox["TexturePath"].IsScalar())
            {
                settings.Skybox.TexturePath = ReadYamlValue(skybox, "TexturePath", settings.Skybox.TexturePath);
            }
            settings.Skybox.Mode = ReadYamlValue(skybox, "Mode", settings.Skybox.Mode);
            settings.Skybox.Exposure = ReadYamlValue(skybox, "Exposure", settings.Skybox.Exposure);
            settings.Skybox.Brightness = ReadYamlValue(skybox, "Brightness", settings.Skybox.Brightness);
            settings.Skybox.Contrast = ReadYamlValue(skybox, "Contrast", settings.Skybox.Contrast);
        }

        // Fog
        if (auto fog = data["Fog"])
        {
            settings.Fog.Enabled = ReadYamlValue(fog, "Enabled", settings.Fog.Enabled);
            settings.Fog.FogColor = ReadYamlValue(fog, "Color", settings.Fog.FogColor);
            settings.Fog.Density = ReadYamlValue(fog, "Density", settings.Fog.Density);
            settings.Fog.Start = ReadYamlValue(fog, "Start", settings.Fog.Start);
            settings.Fog.End = ReadYamlValue(fog, "End", settings.Fog.End);
        }
    }

    auto entities = data["Entities"];
    if (entities && entities.IsSequence())
    {
        std::vector<HierarchyTask> hierarchyTasks;
        std::set<uint64_t> seenUUIDs;

        for (auto entity : entities)
        {
            if (!entity["Entity"])
            {
                continue;
            }

            uint64_t uuid = ReadYamlValue(entity, "Entity", uint64_t{0});
            if (uuid == 0)
            {
                uuid = UUID();
            }
            else if (seenUUIDs.count(uuid))
            {
                uuid = UUID();
            }
            seenUUIDs.insert(uuid);

            std::string name;
            auto tagComponent = entity["Tag"];
            if (tagComponent && tagComponent["Tag"] && tagComponent["Tag"].IsScalar())
            {
                name = ReadYamlValue(tagComponent, "Tag", std::string());
            }

            Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

            // Use ComponentSerializer registry for all components
            ComponentSerializer::Get().DeserializeAll(deserializedEntity, entity);

            // Hierarchy task
            HierarchyTask task;
            HierarchySerializer::DeserializeTask(deserializedEntity, entity, task);
            if (task.entity)
            {
                hierarchyTasks.push_back(task);
            }
        }

        // Phase 3: Finalize Hierarchy
        for (auto& task : hierarchyTasks)
        {
            if (!task.entity.HasComponent<HierarchyComponent>())
            {
                task.entity.AddComponent<HierarchyComponent>();
            }
            auto& hc = task.entity.GetComponent<HierarchyComponent>();
            if (task.parent != 0)
            {
                CHEngine::Entity parent = m_Scene->GetEntityByUUID(task.parent);
                if (parent)
                {
                    hc.Parent = parent;
                }
            }

            for (uint64_t childUUID : task.children)
            {
                CHEngine::Entity child = m_Scene->GetEntityByUUID(childUUID);
                if (child)
                {
                    hc.Children.push_back(child);
                }
            }
        }

        // Phase 4: Preload all model assets
        auto modelView = m_Scene->GetRegistry().view<ModelComponent>();
        auto& assetMgr = AssetManager::Get();
        for (auto entity : modelView)
        {
            auto& modelComp = m_Scene->GetRegistry().get<ModelComponent>(entity);
            if (!modelComp.ModelPath.empty())
            {
                // Trigger asset loading
                assetMgr.Get<ModelAsset>(modelComp.ModelPath);
            }
        }

    }

    return true;
}
} // namespace CHEngine
