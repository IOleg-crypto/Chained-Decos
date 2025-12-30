#include "Project.h"
#include "core/Log.h"
#include <fstream>

namespace CHEngine
{

std::shared_ptr<Project> Project::Create(const std::filesystem::path &directory,
                                         const std::string &name)
{
    auto project = std::shared_ptr<Project>(new Project());
    project->m_Config.name = name;
    project->m_ProjectDirectory = directory / name;
    project->m_ProjectFilePath = project->m_ProjectDirectory / (name + ".chproject");

    // Create directory structure
    if (!project->CreateDirectoryStructure())
    {
        CD_CORE_ERROR("[Project] Failed to create directory structure for project: %s",
                      name.c_str());
        return nullptr;
    }

    // Save initial project file
    if (!project->SerializeToYAML())
    {
        CD_CORE_ERROR("[Project] Failed to serialize project file: %s", name.c_str());
        return nullptr;
    }

    // Create default scene
    std::filesystem::path defaultScenePath = project->GetSceneDirectory() / "DefaultScene.chscene";
    std::ofstream sceneFile(defaultScenePath);
    if (sceneFile.is_open())
    {
        sceneFile << "# Default Scene\n";
        sceneFile.close();
        project->m_Config.startScene = "Scenes/DefaultScene.chscene";
        project->Save();
    }

    CD_CORE_INFO("[Project] Created new project: %s at %s", name.c_str(),
                 project->m_ProjectDirectory.string().c_str());
    return project;
}

std::shared_ptr<Project> Project::Load(const std::filesystem::path &projectFilePath)
{
    if (!std::filesystem::exists(projectFilePath))
    {
        CD_CORE_ERROR("[Project] Project file does not exist: %s",
                      projectFilePath.string().c_str());
        return nullptr;
    }

    auto project = std::shared_ptr<Project>(new Project());
    project->m_ProjectFilePath = projectFilePath;
    project->m_ProjectDirectory = projectFilePath.parent_path();

    if (!project->DeserializeFromYAML())
    {
        CD_CORE_ERROR("[Project] Failed to load project from: %s",
                      projectFilePath.string().c_str());
        return nullptr;
    }

    CD_CORE_INFO("[Project] Loaded project: %s from %s", project->m_Config.name.c_str(),
                 projectFilePath.string().c_str());
    return project;
}

void Project::Save()
{
    if (!SerializeToYAML())
    {
        CD_CORE_ERROR("[Project] Failed to save project: %s", m_Config.name.c_str());
    }
    else
    {
        CD_CORE_INFO("[Project] Saved project: %s", m_Config.name.c_str());
    }
}

std::filesystem::path Project::GetAssetDirectory() const
{
    return m_ProjectDirectory / m_Config.directories.assets;
}

std::filesystem::path Project::GetSceneDirectory() const
{
    return m_ProjectDirectory / m_Config.directories.scenes;
}

std::filesystem::path Project::GetScriptDirectory() const
{
    return m_ProjectDirectory / m_Config.directories.scripts;
}

std::filesystem::path Project::GetCacheDirectory() const
{
    return m_ProjectDirectory / m_Config.directories.cache;
}

std::filesystem::path Project::GetAbsolutePath(const std::string &relativePath) const
{
    return m_ProjectDirectory / relativePath;
}

std::string Project::GetRelativePath(const std::filesystem::path &absolutePath) const
{
    return std::filesystem::relative(absolutePath, m_ProjectDirectory).string();
}

bool Project::CreateDirectoryStructure()
{
    try
    {
        // Create project root
        std::filesystem::create_directories(m_ProjectDirectory);

        // Create subdirectories
        std::filesystem::create_directories(GetAssetDirectory());
        std::filesystem::create_directories(GetAssetDirectory() / "Models");
        std::filesystem::create_directories(GetAssetDirectory() / "Textures");
        std::filesystem::create_directories(GetAssetDirectory() / "Audio");
        std::filesystem::create_directories(GetScriptDirectory());
        std::filesystem::create_directories(GetSceneDirectory());
        std::filesystem::create_directories(GetCacheDirectory());

        return true;
    }
    catch (const std::exception &e)
    {
        CD_CORE_ERROR("[Project] Exception creating directories: %s", e.what());
        return false;
    }
}

bool Project::SerializeToYAML()
{
    try
    {
        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "name" << YAML::Value << m_Config.name;
        out << YAML::Key << "version" << YAML::Value << m_Config.version;
        out << YAML::Key << "engine_version" << YAML::Value << m_Config.engineVersion;
        out << YAML::Key << "start_scene" << YAML::Value << m_Config.startScene;

        out << YAML::Key << "directories" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "assets" << YAML::Value << m_Config.directories.assets;
        out << YAML::Key << "scenes" << YAML::Value << m_Config.directories.scenes;
        out << YAML::Key << "scripts" << YAML::Value << m_Config.directories.scripts;
        out << YAML::Key << "cache" << YAML::Value << m_Config.directories.cache;
        out << YAML::EndMap;

        out << YAML::Key << "settings" << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "physics" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "gravity" << YAML::Value << YAML::Flow << YAML::BeginSeq
            << m_Config.settings.physics.gravity[0] << m_Config.settings.physics.gravity[1]
            << m_Config.settings.physics.gravity[2] << YAML::EndSeq;
        out << YAML::Key << "fixed_timestep" << YAML::Value
            << m_Config.settings.physics.fixedTimestep;
        out << YAML::EndMap;

        out << YAML::Key << "rendering" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "vsync" << YAML::Value << m_Config.settings.rendering.vsync;
        out << YAML::Key << "target_fps" << YAML::Value << m_Config.settings.rendering.targetFPS;
        out << YAML::EndMap;

        out << YAML::Key << "editor" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "grid_size" << YAML::Value << m_Config.settings.editor.gridSize;
        out << YAML::Key << "draw_wireframe" << YAML::Value
            << m_Config.settings.editor.drawWireframe;
        out << YAML::Key << "draw_collisions" << YAML::Value
            << m_Config.settings.editor.drawCollisions;
        out << YAML::EndMap;

        out << YAML::EndMap; // settings
        out << YAML::EndMap; // root

        std::ofstream fout(m_ProjectFilePath);
        fout << out.c_str();
        fout.close();

        return true;
    }
    catch (const std::exception &e)
    {
        CD_CORE_ERROR("[Project] Exception serializing to YAML: %s", e.what());
        return false;
    }
}

bool Project::DeserializeFromYAML()
{
    try
    {
        YAML::Node root = YAML::LoadFile(m_ProjectFilePath.string());

        m_Config.name = root["name"].as<std::string>();
        m_Config.version = root["version"].as<std::string>("1.0.0");
        m_Config.engineVersion = root["engine_version"].as<std::string>("0.1.0");
        m_Config.startScene = root["start_scene"].as<std::string>("");

        if (root["directories"])
        {
            auto dirs = root["directories"];
            m_Config.directories.assets = dirs["assets"].as<std::string>("Assets");
            m_Config.directories.scenes = dirs["scenes"].as<std::string>("Scenes");
            m_Config.directories.scripts = dirs["scripts"].as<std::string>("Assets/Scripts");
            m_Config.directories.cache = dirs["cache"].as<std::string>("Cache");
        }

        if (root["settings"])
        {
            auto settings = root["settings"];

            if (settings["physics"])
            {
                auto physics = settings["physics"];
                if (physics["gravity"] && physics["gravity"].IsSequence())
                {
                    m_Config.settings.physics.gravity[0] = physics["gravity"][0].as<float>(0.0f);
                    m_Config.settings.physics.gravity[1] = physics["gravity"][1].as<float>(-9.81f);
                    m_Config.settings.physics.gravity[2] = physics["gravity"][2].as<float>(0.0f);
                }
                m_Config.settings.physics.fixedTimestep =
                    physics["fixed_timestep"].as<float>(0.02f);
            }

            if (settings["rendering"])
            {
                auto rendering = settings["rendering"];
                m_Config.settings.rendering.vsync = rendering["vsync"].as<bool>(true);
                m_Config.settings.rendering.targetFPS = rendering["target_fps"].as<int>(60);
            }

            if (settings["editor"])
            {
                auto editor = settings["editor"];
                m_Config.settings.editor.gridSize = editor["grid_size"].as<int>(50);
                m_Config.settings.editor.drawWireframe = editor["draw_wireframe"].as<bool>(false);
                m_Config.settings.editor.drawCollisions = editor["draw_collisions"].as<bool>(false);
            }
        }

        return true;
    }
    catch (const std::exception &e)
    {
        CD_CORE_ERROR("[Project] Exception deserializing from YAML: %s", e.what());
        return false;
    }
}

const std::filesystem::path &Project::GetProjectDirectory() const
{
    return m_ProjectDirectory;
}
const std::filesystem::path &Project::GetProjectFilePath() const
{
    return m_ProjectFilePath;
}
Project::ProjectConfig &Project::GetConfig()
{
    return m_Config;
}
const Project::ProjectConfig &Project::GetConfig() const
{
    return m_Config;
}
const std::string &Project::GetName() const
{
    return m_Config.name;
}
} // namespace CHEngine
