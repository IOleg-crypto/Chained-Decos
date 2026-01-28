#include "editor.h"
#include "core/application.h"
#include "editor_layer.h"
#include "engine/core/yaml.h"
#include "raylib.h"
#include <fstream>

namespace CHEngine
{
Editor::Editor(const Application::Config &config) : Application(config)
{
    CH_CORE_INFO("Editor Started");
    LoadConfig();
}

void Editor::PostInitialize()
{
    CH_CORE_INFO("Editor PostInitialize - Setting window icon");
    Application::Config config = GetConfig();
    config.WindowIcon =
        LoadImage(PROJECT_ROOT_DIR
                  "/engine/resources/icons/game-engine-icon-featuring-a-game-controller-with-.png");

    SetWindowIcon(config.WindowIcon);

    CH_CORE_INFO("Editor PostInitialize - Pushing EditorLayer");
    PushLayer(new EditorLayer());
}

void Editor::Shutdown()
{
    SaveConfig();
}

void Editor::LoadConfig()
{
    std::string configPath = "editor.yaml";
    if (!std::filesystem::exists(configPath))
        return;

    try
    {
        YAML::Node data = YAML::LoadFile(configPath);
        if (!data["Editor"])
            return;

        auto node = data["Editor"];
        m_EditorConfig.WindowWidth = node["WindowWidth"].as<int>(1600);
        m_EditorConfig.WindowHeight = node["WindowHeight"].as<int>(900);
        m_EditorConfig.Fullscreen = node["Fullscreen"].as<bool>(false);
        m_EditorConfig.TargetFPS = node["TargetFPS"].as<int>(144);
        m_EditorConfig.LastProjectPath = node["LastProjectPath"].as<std::string>("");
        m_EditorConfig.LastScenePath = node["LastScenePath"].as<std::string>("");
    }
    catch (std::exception &e)
    {
        CH_CORE_ERROR("Failed to load editor config: {}", e.what());
    }
}

void Editor::SaveConfig()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Editor" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "WindowWidth" << YAML::Value << m_EditorConfig.WindowWidth;
    out << YAML::Key << "WindowHeight" << YAML::Value << m_EditorConfig.WindowHeight;
    out << YAML::Key << "Fullscreen" << YAML::Value << m_EditorConfig.Fullscreen;
    out << YAML::Key << "TargetFPS" << YAML::Value << m_EditorConfig.TargetFPS;
    out << YAML::Key << "LastProjectPath" << YAML::Value << m_EditorConfig.LastProjectPath;
    out << YAML::Key << "LastScenePath" << YAML::Value << m_EditorConfig.LastScenePath;
    out << YAML::EndMap;
    out << YAML::EndMap;

    std::ofstream fout("editor.yaml");
    fout << out.c_str();
}

} // namespace CHEngine
