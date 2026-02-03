#include "project.h"
#include "project_serializer.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/environment.h"
#include "imgui.h"

namespace CHEngine
{
bool PhysicsSettings::DrawUI()
{
    bool changed = false;
    if (ImGui::DragFloat("World Gravity", &Gravity, 0.1f, 0.0f, 100.0f))
    {
        changed = true;
    }
    return changed;
}

bool AnimationSettings::DrawUI()
{
    bool changed = false;
    if (ImGui::DragFloat("Target FPS", &TargetFPS, 1.0f, 1.0f, 240.0f))
    {
        changed = true;
    }
    return changed;
}

bool RenderSettings::DrawUI()
{
    bool changed = false;
    if (ImGui::DragFloat("Ambient Intensity", &AmbientIntensity, 0.01f, 0.0f, 1.0f))
    {
        changed = true;
    }
    if (ImGui::DragFloat("Default Exposure", &DefaultExposure, 0.01f, 0.0f, 10.0f))
    {
        changed = true;
    }
    return changed;
}

bool WindowSettings::DrawUI()
{
    bool changed = false;
    if (ImGui::DragInt("Width", &Width, 1, 800, 3840))
    {
        changed = true;
    }
    if (ImGui::DragInt("Height", &Height, 1, 600, 2160))
    {
        changed = true;
    }
    if (ImGui::Checkbox("VSync", &VSync))
    {
        changed = true;
    }
    if (ImGui::Checkbox("Resizable", &Resizable))
    {
        changed = true;
    }
    return changed;
}

bool RuntimeSettings::DrawUI()
{
    bool changed = false;
    if (ImGui::Checkbox("Fullscreen", &Fullscreen))
    {
        changed = true;
    }
    if (ImGui::Checkbox("Show Stats", &ShowStats))
    {
        changed = true;
    }
    if (ImGui::Checkbox("Enable Console", &EnableConsole))
    {
        changed = true;
    }
    return changed;
}

bool EditorSettings::DrawUI()
{
    bool changed = false;
    if (ImGui::DragFloat("Camera Speed", &CameraMoveSpeed, 0.1f, 0.1f, 100.0f))
    {
        changed = true;
    }
    if (ImGui::DragFloat("Rotation Speed", &CameraRotationSpeed, 0.01f, 0.01f, 1.0f))
    {
        changed = true;
    }
    if (ImGui::DragFloat("Boost Multiplier", &CameraBoostMultiplier, 0.1f, 1.0f, 20.0f))
    {
        changed = true;
    }
    return changed;
}

std::shared_ptr<Project> Project::s_ActiveProject = nullptr;

std::shared_ptr<Project> Project::New()
{
    auto project = std::make_shared<Project>();
    project->m_AssetManager = std::make_shared<AssetManager>();
    project->m_AssetManager->Initialize();
    s_ActiveProject = project;
    return s_ActiveProject;
}

std::shared_ptr<Project> Project::Load(const std::filesystem::path &path)
{
    std::shared_ptr<Project> project = std::make_shared<Project>();
    project->m_AssetManager = std::make_shared<AssetManager>();
    project->m_AssetManager->Initialize(path.parent_path());
    
    ProjectSerializer serializer(project);
    if (serializer.Deserialize(path))
    {
        project->m_Config.ProjectDirectory = path.parent_path();
        
        // Register asset search path
        project->m_AssetManager->ClearSearchPaths();
        project->m_AssetManager->AddSearchPath(project->m_Config.ProjectDirectory / project->m_Config.AssetDirectory);
        
        // Load environment if specified
        if (!project->m_Config.EnvironmentPath.empty())
        {
            project->m_Environment = project->m_AssetManager->Get<EnvironmentAsset>(project->m_Config.EnvironmentPath.string());
        }

        s_ActiveProject = project;
        return s_ActiveProject;
    }

    return nullptr;
}

bool Project::SaveActive(const std::filesystem::path &path)
{
    ProjectSerializer serializer(s_ActiveProject);
    if (serializer.Serialize(path))
    {
        s_ActiveProject->m_Config.ProjectDirectory = path.parent_path();
        return true;
    }

    return false;
}
    std::vector<std::string> Project::GetAvailableScenes()
    {
        std::vector<std::string> scenes;
        if (!s_ActiveProject)
            return scenes;

        auto assetDir = GetAssetDirectory();
        auto scenesDir = assetDir / "scenes";

        if (std::filesystem::exists(scenesDir))
        {
            for (auto &entry : std::filesystem::recursive_directory_iterator(scenesDir))
            {
                if (entry.path().extension() == ".chscene")
                {
                    std::string relPath = std::filesystem::relative(entry.path(), assetDir).string();
                    scenes.push_back(relPath);
                }
            }
        }
        return scenes;
    }
} // namespace CHEngine
