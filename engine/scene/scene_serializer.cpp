#include "scene_serializer.h"
#include "engine/core/service_locator.h"
#include "component_serializer.h"
#include "engine/assets/asset_manager.h"
#include "engine/project/project.h"
#include "engine/scene/yaml.h"
#include "engine/assets/types/environment_asset.h"
#include "scene.h"
#include "engine/scene/hierarchy_serializer.h"
#include <fstream>
#include <set>
#include <sstream>

namespace Chained
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

namespace
{
static std::string ToProjectRelativePath(const std::string& absPath)
{
    auto project = Project::GetActive();
    return project ? project->GetRelativePathForProject(absPath) : absPath;
}
} // namespace

static void SerializeBackgroundSettings(YAML::Emitter& out, const SceneSettings& settings)
{
    out << YAML::Key << "Background" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Mode" << YAML::Value << (int)settings.Mode;
    out << YAML::Key << "Color" << YAML::Value << settings.BackgroundColor;
    out << YAML::Key << "TexturePath" << YAML::Value << settings.BackgroundTexturePath;
    out << YAML::EndMap;
}

static void SerializeCanvasSettings(YAML::Emitter& out, const SceneSettings& settings)
{
    out << YAML::Key << "Canvas" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "ReferenceResolution" << YAML::Value << settings.Canvas.ReferenceResolution;
    out << YAML::Key << "ScaleMode" << YAML::Value << (int)settings.Canvas.ScaleMode;
    out << YAML::Key << "MatchWidthOrHeight" << YAML::Value << settings.Canvas.MatchWidthOrHeight;
    out << YAML::EndMap;
}

static void SerializeEnvironmentSettings(YAML::Emitter& out, const SceneSettings& settings)
{
    if (settings.Environment)
    {
        std::string envPath = ToProjectRelativePath(settings.Environment->GetPath());
        out << YAML::Key << "EnvironmentPath" << YAML::Value << envPath;
    }

    // Also serialize the current settings for quick preview/fallback.
    if (!settings.Environment)
    {
        return;
    }

    const auto& envSettings = settings.Environment->GetSettings();

    out << YAML::Key << "Lighting" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Direction" << YAML::Value << envSettings.Lighting.Direction;
    out << YAML::Key << "LightColor" << YAML::Value << envSettings.Lighting.LightColor;
    out << YAML::Key << "Ambient" << YAML::Value << envSettings.Lighting.Ambient;
    out << YAML::EndMap;

    out << YAML::Key << "Skybox" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "TexturePath" << YAML::Value << ToProjectRelativePath(envSettings.Skybox.TexturePath);
    out << YAML::Key << "Mode" << YAML::Value << envSettings.Skybox.Mode;
    out << YAML::Key << "Exposure" << YAML::Value << envSettings.Skybox.Exposure;
    out << YAML::Key << "Brightness" << YAML::Value << envSettings.Skybox.Brightness;
    out << YAML::Key << "Contrast" << YAML::Value << envSettings.Skybox.Contrast;
    out << YAML::EndMap;

    out << YAML::Key << "Fog" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Enabled" << YAML::Value << envSettings.Fog.Enabled;
    out << YAML::Key << "Color" << YAML::Value << envSettings.Fog.FogColor;
    out << YAML::Key << "Density" << YAML::Value << envSettings.Fog.Density;
    out << YAML::Key << "Start" << YAML::Value << envSettings.Fog.Start;
    out << YAML::Key << "End" << YAML::Value << envSettings.Fog.End;
    out << YAML::EndMap;
}

static void SerializeDebugSettings(YAML::Emitter& out, const SceneSettings& settings)
{
    out << YAML::Key << "DebugSettings" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "DiagnosticMode" << YAML::Value << settings.DiagnosticMode;
    out << YAML::Key << "DrawColliders" << YAML::Value << settings.DebugFlags.DrawColliders;
    out << YAML::Key << "DrawHierarchy" << YAML::Value << settings.DebugFlags.DrawHierarchy;
    out << YAML::Key << "DrawGrid" << YAML::Value << settings.DebugFlags.DrawGrid;
    out << YAML::Key << "DrawSelection" << YAML::Value << settings.DebugFlags.DrawSelection;
    out << YAML::Key << "DrawLights" << YAML::Value << settings.DebugFlags.DrawLights;
    out << YAML::Key << "DrawSpawnZones" << YAML::Value << settings.DebugFlags.DrawSpawnZones;
    out << YAML::Key << "CollisionWireframeMode" << YAML::Value << settings.DebugFlags.SetCollisionWireframeMode;
    out << YAML::EndMap;
}

static void DeserializeBackgroundSettings(const YAML::Node& data, SceneSettings& settings)
{
    if (!data["Background"])
    {
        return;
    }

    auto background = data["Background"];
    settings.Mode = static_cast<BackgroundMode>(ReadYamlValue(background, "Mode", static_cast<int>(settings.Mode)));
    settings.BackgroundColor = ReadYamlValue(background, "Color", settings.BackgroundColor);
    if (background["TexturePath"] && background["TexturePath"].IsScalar())
    {
        settings.BackgroundTexturePath = ReadYamlValue(background, "TexturePath", settings.BackgroundTexturePath);
    }
    // Legacy AmbientIntensity in Background block is silently ignored.
}

static void DeserializeCanvasSettings(const YAML::Node& data, SceneSettings& settings)
{
    if (!data["Canvas"])
    {
        return;
    }

    auto canvas = data["Canvas"];
    settings.Canvas.ReferenceResolution =
        ReadYamlValue(canvas, "ReferenceResolution", settings.Canvas.ReferenceResolution);
    settings.Canvas.ScaleMode = static_cast<CanvasScaleMode>(
        ReadYamlValue(canvas, "ScaleMode", static_cast<int>(settings.Canvas.ScaleMode)));
    settings.Canvas.MatchWidthOrHeight =
        ReadYamlValue(canvas, "MatchWidthOrHeight", settings.Canvas.MatchWidthOrHeight);
}

static void DeserializeDebugSettings(const YAML::Node& data, SceneSettings& settings)
{
    if (!data["DebugSettings"])
    {
        return;
    }

    auto debugNode = data["DebugSettings"];
    settings.DiagnosticMode = ReadYamlValue(debugNode, "DiagnosticMode", 0.0f);
    settings.DebugFlags.DrawColliders = ReadYamlValue(debugNode, "DrawColliders", false);
    settings.DebugFlags.DrawHierarchy = ReadYamlValue(debugNode, "DrawHierarchy", false);
    settings.DebugFlags.DrawGrid = ReadYamlValue(debugNode, "DrawGrid", false);
    settings.DebugFlags.DrawSelection = ReadYamlValue(debugNode, "DrawSelection", true);
    settings.DebugFlags.DrawLights = ReadYamlValue(debugNode, "DrawLights", true);
    settings.DebugFlags.DrawSpawnZones = ReadYamlValue(debugNode, "DrawSpawnZones", true);
    settings.DebugFlags.SetCollisionWireframeMode = ReadYamlValue(debugNode, "CollisionWireframeMode", 0);
}

static void DeserializeEnvironmentSettings(const YAML::Node& data, SceneSettings& settings)
{
    if (data["EnvironmentPath"] && data["EnvironmentPath"].IsScalar())
    {
        std::string envPath = ReadYamlValue(data, "EnvironmentPath", std::string());
        if (Project::GetActive())
        {
            auto handle = ServiceLocator::Get<AssetManager>()->LoadAsset(envPath , EnvironmentAsset::GetStaticType());
            auto sharedEnv = ServiceLocator::Get<AssetManager>()->Get<EnvironmentAsset>(envPath);
            if (sharedEnv)
            {
                if (!settings.Environment)
                    settings.Environment = std::make_shared<EnvironmentAsset>();

                settings.Environment->SetSettings(sharedEnv->GetSettings());
                settings.Environment->SetPath(sharedEnv->GetPath());
            }
        }
    }

    if (!data["Skybox"] && !data["Fog"] && !data["LightDirection"])
    {
        return;
    }

    if (!settings.Environment)
    {
        settings.Environment = std::make_shared<EnvironmentAsset>();
    }

    auto& env = settings.Environment;
    auto& envSettings = env->GetSettings();

    if (data["Lighting"])
    {
        auto lighting = data["Lighting"];
        envSettings.Lighting.Direction = ReadYamlValue(lighting, "Direction", envSettings.Lighting.Direction);
        envSettings.Lighting.LightColor = ReadYamlValue(lighting, "LightColor", envSettings.Lighting.LightColor);
        envSettings.Lighting.Ambient = ReadYamlValue(lighting, "Ambient", envSettings.Lighting.Ambient);
    }
    else
    {
        // Backward compat: old flat field names.
        envSettings.Lighting.Direction = ReadYamlValue(data, "LightDirection", envSettings.Lighting.Direction);
        envSettings.Lighting.LightColor = ReadYamlValue(data, "LightColor", envSettings.Lighting.LightColor);
        envSettings.Lighting.Ambient = ReadYamlValue(data, "AmbientIntensity", envSettings.Lighting.Ambient);
    }

    if (auto skybox = data["Skybox"])
    {
        if (skybox["TexturePath"] && skybox["TexturePath"].IsScalar())
        {
            envSettings.Skybox.TexturePath = ReadYamlValue(skybox, "TexturePath", envSettings.Skybox.TexturePath);
        }
        envSettings.Skybox.Mode = ReadYamlValue(skybox, "Mode", envSettings.Skybox.Mode);
        envSettings.Skybox.Exposure = ReadYamlValue(skybox, "Exposure", envSettings.Skybox.Exposure);
        envSettings.Skybox.Brightness = ReadYamlValue(skybox, "Brightness", envSettings.Skybox.Brightness);
        envSettings.Skybox.Contrast = ReadYamlValue(skybox, "Contrast", envSettings.Skybox.Contrast);
    }

    if (auto fog = data["Fog"])
    {
        envSettings.Fog.Enabled = ReadYamlValue(fog, "Enabled", envSettings.Fog.Enabled);
        envSettings.Fog.FogColor = ReadYamlValue(fog, "Color", envSettings.Fog.FogColor);
        envSettings.Fog.Density = ReadYamlValue(fog, "Density", envSettings.Fog.Density);
        envSettings.Fog.Start = ReadYamlValue(fog, "Start", envSettings.Fog.Start);
        envSettings.Fog.End = ReadYamlValue(fog, "End", envSettings.Fog.End);
    }
}

static void SerializeSceneSettings(YAML::Emitter& out, const SceneSettings& settings)
{
    out << YAML::Key << "Scene" << YAML::Value << settings.Name;
    SerializeBackgroundSettings(out, settings);
    SerializeCanvasSettings(out, settings);
    SerializeEnvironmentSettings(out, settings);
    SerializeDebugSettings(out, settings);
}

static bool DeserializeSceneSettings(const YAML::Node& data, SceneSettings& settings, std::string& lastError)
{
    if (!data["Scene"] || !data["Scene"].IsScalar())
    {
        lastError = "SceneSerializer: missing Scene root key";
        return false;
    }

    settings.Name = ReadYamlValue(data, "Scene", settings.Name);
    DeserializeBackgroundSettings(data, settings);
    DeserializeCanvasSettings(data, settings);
    DeserializeDebugSettings(data, settings);
    DeserializeEnvironmentSettings(data, settings);
    return true;
}

static void DeserializeEntities(Scene* scene, const YAML::Node& entities)
{
    std::vector<HierarchyTask> hierarchyTasks;
    std::set<uint64_t> seenUUIDs;

    for (auto entity : entities)
    {
        if (!entity["Entity"])
        {
            CH_CORE_WARN("SceneSerializer: Malformed entity entry missing 'Entity' key, skipping.");
            continue;
        }

        uint64_t uuid = ReadYamlValue(entity, "Entity", uint64_t{0});
        if (uuid == 0)
        {
            uuid = UUID();
        }
        else if (seenUUIDs.count(uuid))
        {
            CH_CORE_WARN("SceneSerializer: Duplicate UUID {} found, generating new UUID.", uuid);
            uuid = UUID();
        }
        seenUUIDs.insert(uuid);

        std::string name;
        auto tagComponent = entity["Tag"];
        if (tagComponent && tagComponent["Tag"] && tagComponent["Tag"].IsScalar())
        {
            name = ReadYamlValue(tagComponent, "Tag", std::string());
        }

        Entity deserializedEntity = scene->CreateEntityWithUUID(uuid, name);

        ComponentSerializer::DeserializeAll(deserializedEntity, entity);

        if (deserializedEntity.HasComponent<TransformComponent>())
        {
            auto& tc = deserializedEntity.GetComponent<TransformComponent>();
            tc.RotationQuat = glm::quat(tc.Rotation);
            tc.PrevRotationQuat = tc.RotationQuat;
            tc.PrevTranslation = tc.Translation;
            tc.PrevScale = tc.Scale;
        }

        HierarchyTask task;
        HierarchySerializer::DeserializeTask(deserializedEntity, entity, task);
        if (task.entity)
        {
            hierarchyTasks.push_back(task);
        }
    }

    for (auto& task : hierarchyTasks)
    {
        if (!task.entity.HasComponent<HierarchyComponent>())
        {
            task.entity.AddComponent<HierarchyComponent>();
        }

        auto& hc = task.entity.GetComponent<HierarchyComponent>();
        if (task.parent != 0)
        {
            Chained::Entity parent = scene->GetEntityByUUID(task.parent);
            if (parent)
            {
                hc.Parent = parent;
            }
        }

        for (uint64_t childUUID : task.children)
        {
            Chained::Entity child = scene->GetEntityByUUID(childUUID);
            if (child)
            {
                hc.Children.push_back(child);
            }
        }
    }
}
} // namespace

static void SerializeEntity(YAML::Emitter& out, Entity entity)
{
    out << YAML::BeginMap; // Entity

    ComponentSerializer::SerializeID(out, entity);
    ComponentSerializer::SerializeAll(out, entity);

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

    SerializeSceneSettings(out, m_Scene->GetSettings());

    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

    m_Scene->GetRegistry().view<IDComponent>().each([&, this](auto entityID, auto& id) {
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
        if (!fout.good())
        {
            CH_CORE_ERROR("SceneSerializer: Failed to write scene file '{}'.", filepath);
            return false;
        }
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

    if (!DeserializeSceneSettings(data, m_Scene->GetSettings(), m_LastError))
    {
        return false;
    }

    auto entities = data["Entities"];
    if (entities && entities.IsSequence())
    {
        DeserializeEntities(m_Scene, entities);
    }

    return true;
}
} // namespace Chained
