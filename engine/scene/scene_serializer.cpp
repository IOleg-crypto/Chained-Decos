#include "scene_serializer.h"
#include "component_serializer.h"
#include "components.h"
#include "engine/core/log.h"
#include "engine/core/yaml.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/model_asset.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/scene/project.h"
#include "scene.h"
#include "scriptable_entity.h"
#include <fstream>
#include "yaml-cpp/yaml.h"

namespace CHEngine
{
SceneSerializer::SceneSerializer(Scene* scene)
    : m_Scene(scene)
{
}

void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
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
    out << YAML::Key << "DrawCollisionModelBox" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawCollisionModelBox;
    out << YAML::Key << "DrawGrid" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawGrid;
    out << YAML::Key << "DrawSelection" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawSelection;
    out << YAML::Key << "DrawLights" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawLights;
    out << YAML::Key << "DrawSpawnZones" << YAML::Value << m_Scene->GetSettings().DebugFlags.DrawSpawnZones;
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
        CH_CORE_INFO("Scene saved successfully to: {}", filepath.c_str());
        return true;
    }
    else
    {
        CH_CORE_ERROR("Failed to save scene to: {}", filepath.c_str());
        return false;
    }
}

bool SceneSerializer::Deserialize(const std::string& filepath)
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

bool SceneSerializer::DeserializeFromString(const std::string& yaml)
{
    try
    {
        YAML::Node data = YAML::Load(yaml);
        if (!data["Scene"])
        {
            return false;
        }

        std::string sceneName = data["Scene"].as<std::string>();
        CH_CORE_INFO("Deserializing scene '{}'", sceneName.c_str());

        // Deserialize Background
        if (data["Background"])
        {
            auto background = data["Background"];
            if (background["Mode"])
            {
                m_Scene->GetSettings().Mode = (BackgroundMode)background["Mode"].as<int>();
            }
            if (background["Color"])
            {
                m_Scene->GetSettings().BackgroundColor = background["Color"].as<Color>();
            }
            if (background["TexturePath"] && background["TexturePath"].IsScalar())
            {
                m_Scene->GetSettings().BackgroundTexturePath = background["TexturePath"].as<std::string>();
            }
            // Legacy AmbientIntensity in Background block is silently ignored
        }

        // Deserialize Canvas
        if (data["Canvas"])
        {
            auto canvas = data["Canvas"];
            if (canvas["ReferenceResolution"])
            {
                m_Scene->GetSettings().Canvas.ReferenceResolution = canvas["ReferenceResolution"].as<Vector2>();
            }
            if (canvas["ScaleMode"])
            {
                m_Scene->GetSettings().Canvas.ScaleMode = (CanvasScaleMode)canvas["ScaleMode"].as<int>();
            }
            if (canvas["MatchWidthOrHeight"])
            {
                m_Scene->GetSettings().Canvas.MatchWidthOrHeight = canvas["MatchWidthOrHeight"].as<float>();
            }
        }

        // Deserialize Debug Settings
        if (data["DebugSettings"])
        {
            auto debugNode = data["DebugSettings"];
            m_Scene->GetSettings().DiagnosticMode = debugNode["DiagnosticMode"].as<float>(0.0f);
            m_Scene->GetSettings().DebugFlags.DrawColliders = debugNode["DrawColliders"].as<bool>(false);
            m_Scene->GetSettings().DebugFlags.DrawHierarchy = debugNode["DrawHierarchy"].as<bool>(false);
            m_Scene->GetSettings().DebugFlags.DrawCollisionModelBox = debugNode["DrawCollisionModelBox"].as<bool>(false);
            m_Scene->GetSettings().DebugFlags.DrawGrid = debugNode["DrawGrid"].as<bool>(false);
            m_Scene->GetSettings().DebugFlags.DrawSelection = debugNode["DrawSelection"].as<bool>(true);
            m_Scene->GetSettings().DebugFlags.DrawLights = debugNode["DrawLights"].as<bool>(true);
            m_Scene->GetSettings().DebugFlags.DrawSpawnZones = debugNode["DrawSpawnZones"].as<bool>(true);
        }

        // Deserialize Environment
        if (data["EnvironmentPath"] && data["EnvironmentPath"].IsScalar())
        {
            std::string envPath = data["EnvironmentPath"].as<std::string>();
            if (auto project = Project::GetActive())
            {
                m_Scene->GetSettings().Environment = project->GetAssetManager()->Get<EnvironmentAsset>(envPath);
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
                if (lighting["Direction"])
                {
                    settings.Lighting.Direction = lighting["Direction"].as<Vector3>();
                }
                if (lighting["LightColor"])
                {
                    settings.Lighting.LightColor = lighting["LightColor"].as<Color>();
                }
                if (lighting["Ambient"])
                {
                    settings.Lighting.Ambient = lighting["Ambient"].as<float>();
                }
            }
            else
            {
                // Backward compat: old flat field names
                if (data["LightDirection"])
                {
                    settings.Lighting.Direction = data["LightDirection"].as<Vector3>();
                }
                if (data["LightColor"])
                {
                    settings.Lighting.LightColor = data["LightColor"].as<Color>();
                }
                if (data["AmbientIntensity"])
                {
                    settings.Lighting.Ambient = data["AmbientIntensity"].as<float>();
                }
            }

            // Skybox
            if (auto skybox = data["Skybox"])
            {
                if (skybox["TexturePath"] && skybox["TexturePath"].IsScalar())
                {
                    settings.Skybox.TexturePath = skybox["TexturePath"].as<std::string>();
                }
                if (skybox["Mode"])
                {
                    settings.Skybox.Mode = skybox["Mode"].as<int>();
                }
                if (skybox["Exposure"])
                {
                    settings.Skybox.Exposure = skybox["Exposure"].as<float>();
                }
                if (skybox["Brightness"])
                {
                    settings.Skybox.Brightness = skybox["Brightness"].as<float>();
                }
                if (skybox["Contrast"])
                {
                    settings.Skybox.Contrast = skybox["Contrast"].as<float>();
                }
            }

            // Fog
            if (auto fog = data["Fog"])
            {
                if (fog["Enabled"])
                {
                    settings.Fog.Enabled = fog["Enabled"].as<bool>();
                }
                if (fog["Color"])
                {
                    settings.Fog.FogColor = fog["Color"].as<Color>();
                }
                if (fog["Density"])
                {
                    settings.Fog.Density = fog["Density"].as<float>();
                }
                if (fog["Start"])
                {
                    settings.Fog.Start = fog["Start"].as<float>();
                }
                if (fog["End"])
                {
                    settings.Fog.End = fog["End"].as<float>();
                }
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
                if (tagComponent && tagComponent["Tag"] && tagComponent["Tag"].IsScalar())
                {
                    name = tagComponent["Tag"].as<std::string>();
                }

                CH_CORE_TRACE("Deserialized entity with ID = {}, name = {}", uuid, name.c_str());

                Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

                // Use ComponentSerializer registry for all components
                ComponentSerializer::Get().DeserializeAll(deserializedEntity, entity);

                // Hierarchy task
                HierarchyTask task;
                ComponentSerializer::Get().DeserializeHierarchyTask(deserializedEntity, entity, task);
                if (task.entity)
                {
                    hierarchyTasks.push_back(task);
                }

                // Signal is automatically triggered by ComponentSerializer's Patch call
                // No need for manual invocation anymore
                if (deserializedEntity.HasComponent<ModelComponent>())
                {
                    CH_CORE_TRACE("SceneSerializer: ModelComponent deserialized for entity '{}'", name.c_str());
                }
            }

            // Phase 3: Finalize Hierarchy
            for (auto& task : hierarchyTasks)
            {
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
        }
    } catch (const std::exception& e)
    {
        CH_CORE_ERROR("SceneSerializer: FAILED to deserialize scene: {}", e.what());
        return false;
    }

    return true;
}
} // namespace CHEngine
  // namespace CHEngine
