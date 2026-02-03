#include "editor.h"
#include "core/application.h"
#include "editor_layer.h"
#include "engine/core/yaml.h"
#include "raylib.h"
#include <fstream>
#include <filesystem>

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
        Image icon = LoadImage(PROJECT_ROOT_DIR
                               "/engine/resources/icons/game-engine-icon-featuring-a-game-controller-with-.png");
        if (icon.data != nullptr)
            ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

        SetWindowIcon(icon);
        UnloadImage(icon);

        CH_CORE_INFO("Editor PostInitialize - Pushing EditorLayer");
        PushLayer(new EditorLayer());
    }

    void Editor::Shutdown()
    {
        SaveConfig();
    }

    void Editor::LoadConfig()
    {
        std::string configPath = PROJECT_ROOT_DIR "/editor_settings.yaml";
        if (!std::filesystem::exists(configPath))
            return;

        try
        {
            YAML::Node data = YAML::LoadFile(configPath);
            if (!data["Editor"])
                return;

            auto node = data["Editor"];
            m_EditorConfig.LastProjectPath = node["LastProjectPath"].as<std::string>("");
            m_EditorConfig.LastScenePath = node["LastScenePath"].as<std::string>("");
            m_EditorConfig.LoadLastProjectOnStartup = node["LoadLastProjectOnStartup"].as<bool>(false);
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
        out << YAML::Key << "LastProjectPath" << YAML::Value << m_EditorConfig.LastProjectPath;
        out << YAML::Key << "LastScenePath" << YAML::Value << m_EditorConfig.LastScenePath;
        out << YAML::Key << "LoadLastProjectOnStartup" << YAML::Value << m_EditorConfig.LoadLastProjectOnStartup;
        out << YAML::EndMap;
        out << YAML::EndMap;

        std::ofstream fout(PROJECT_ROOT_DIR "/editor_settings.yaml");
        fout << out.c_str();
    }

} // namespace CHEngine
