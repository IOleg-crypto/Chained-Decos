#include "scene_serializer.h"
#include "component_serializer.h"
#include "components.h"
#include "engine/core/log.h"
#include "engine/core/yaml.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/asset_manager.h"
#include "engine/scene/project.h"
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
        
        ComponentSerializer::SerializeID(out, entity);
        ComponentSerializer::SerializeAll(out, entity);

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
        out << YAML::Key << "Background" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Mode" << YAML::Value << (int)m_Scene->m_Settings.Mode;
        out << YAML::Key << "Color" << YAML::Value << m_Scene->m_Settings.BackgroundColor;
        out << YAML::Key << "TexturePath" << YAML::Value << m_Scene->m_Settings.BackgroundTexturePath;
        out << YAML::EndMap;

        // Serialize Environment
        if (m_Scene->m_Settings.Environment)
        {
            out << YAML::Key << "EnvironmentPath" << YAML::Value << m_Scene->m_Settings.Environment->GetPath();
            
            // Also serialize the current settings for quick preview/fallback
            auto& settings = m_Scene->m_Settings.Environment->GetSettings();
            out << YAML::Key << "Skybox" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "TexturePath" << YAML::Value << settings.Skybox.TexturePath;
            out << YAML::Key << "Exposure" << YAML::Value << settings.Skybox.Exposure;
            out << YAML::Key << "Brightness" << YAML::Value << settings.Skybox.Brightness;
            out << YAML::Key << "Contrast" << YAML::Value << settings.Skybox.Contrast;
            out << YAML::EndMap;
        }
        else
        {
            // Fallback: If no environment asset, we might still have a default or temporary skybox
            // (Though in this engine, Environment objects are usually used)
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
        try
        {
            YAML::Node data = YAML::Load(yaml);
            if (!data["Scene"])
                return false;

            std::string sceneName = data["Scene"].as<std::string>();
            CH_CORE_INFO("Deserializing scene '{}'", sceneName.c_str());

            // Deserialize Background
            if (data["Background"])
            {
                auto background = data["Background"];
                if (background["Mode"])
                    m_Scene->m_Settings.Mode = (BackgroundMode)background["Mode"].as<int>();
                if (background["Color"])
                    m_Scene->m_Settings.BackgroundColor = background["Color"].as<Color>();
                if (background["TexturePath"] && background["TexturePath"].IsScalar())
                    m_Scene->m_Settings.BackgroundTexturePath = background["TexturePath"].as<std::string>();
            }

            // Deserialize Environment
            if (data["EnvironmentPath"] && data["EnvironmentPath"].IsScalar())
            {
                std::string envPath = data["EnvironmentPath"].as<std::string>();
                if (auto project = Project::GetActive())
                    m_Scene->m_Settings.Environment = project->GetAssetManager()->Get<EnvironmentAsset>(envPath);
            }

            // Deserialize Skybox (legacy migration)
            if (data["Skybox"])
            {
                // Only migrate if we haven't already loaded an EnvironmentAsset
                if (!m_Scene->m_Settings.Environment)
                {
                    CH_CORE_WARN("SceneSerializer: Migrating legacy Skybox to EnvironmentAsset...");
                    auto skybox = data["Skybox"];
                    
                    auto env = std::make_shared<EnvironmentAsset>();
                    auto& settings = env->GetSettings();
                    
                    if (skybox["TexturePath"] && skybox["TexturePath"].IsScalar())
                        settings.Skybox.TexturePath = skybox["TexturePath"].as<std::string>();
                    if (skybox["Exposure"])
                        settings.Skybox.Exposure = skybox["Exposure"].as<float>();
                    if (skybox["Brightness"])
                        settings.Skybox.Brightness = skybox["Brightness"].as<float>();
                    if (skybox["Contrast"])
                        settings.Skybox.Contrast = skybox["Contrast"].as<float>();
                        
                    // Initialize default environment settings for migration
                    settings.AmbientIntensity = 0.5f; 
                    settings.LightColor = {255, 255, 255, 255};
                    settings.LightDirection = {-0.5f, -1.0f, -0.5f};

                    m_Scene->GetSettings().Environment = env;
                }
            }

            // Deserialize Canvas
            if (data["Canvas"])
            {
                auto canvas = data["Canvas"];
                auto &c = m_Scene->m_Settings.Canvas;
                if (canvas["ReferenceResolution"])
                    c.ReferenceResolution = canvas["ReferenceResolution"].as<glm::vec2>();
                if (canvas["ScaleMode"])
                    c.ScaleMode = (CanvasScaleMode)canvas["ScaleMode"].as<int>();
                if (canvas["MatchWidthOrHeight"])
                    c.MatchWidthOrHeight = canvas["MatchWidthOrHeight"].as<float>();
            }

            auto entities = data["Entities"];
            if (entities && entities.IsSequence())
            {
                std::vector<HierarchyTask> hierarchyTasks;
                std::set<uint64_t> seenUUIDs;

                for (auto entity : entities)
                {
                    if (!entity["Entity"])
                        continue;
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
                        name = tagComponent["Tag"].as<std::string>();

                    CH_CORE_TRACE("Deserialized entity with ID = {}, name = {}", uuid, name.c_str());

                    Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

                    // Use ComponentSerializer registry for all components
                    ComponentSerializer::DeserializeAll(deserializedEntity, entity);

                    // Hierarchy task
                    HierarchyTask task;
                    ComponentSerializer::DeserializeHierarchyTask(deserializedEntity, entity, task);
                    if (task.entity)
                        hierarchyTasks.push_back(task);

                    // Special handling for ModelComponent initial state
                    if (deserializedEntity.HasComponent<ModelComponent>())
                    {
                        m_Scene->OnModelComponentAdded(m_Scene->GetRegistry(), deserializedEntity);
                    }
                }

                // Phase 3: Finalize Hierarchy
                for (auto &task : hierarchyTasks)
                {
                    auto &hc = task.entity.AddComponent<HierarchyComponent>();
                    if (task.parent != 0)
                    {
                        CHEngine::Entity parent = m_Scene->GetEntityByUUID(task.parent);
                        if (parent)
                            hc.Parent = parent;
                    }

                    for (uint64_t childUUID : task.children)
                    {
                        CHEngine::Entity child = m_Scene->GetEntityByUUID(childUUID);
                        if (child)
                            hc.Children.push_back(child);
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            CH_CORE_ERROR("SceneSerializer: FAILED to deserialize scene: {}", e.what());
            return false;
        }

        return true;
    }
}
 // namespace CHEngine
