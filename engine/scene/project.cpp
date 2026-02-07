#include "project.h"
#include "project_serializer.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/environment.h"
#include "imgui.h"

namespace CHEngine
{
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
    
    project->m_Config.ProjectDirectory = path.parent_path();
    s_ActiveProject = project;

    ProjectSerializer serializer(project);
    if (serializer.Deserialize(path))
    {
        // Register asset search path
        project->m_AssetManager->ClearSearchPaths();
        project->m_AssetManager->AddSearchPath(project->m_Config.ProjectDirectory / project->m_Config.AssetDirectory);
        
        // Load environment if specified
        if (!project->m_Config.EnvironmentPath.empty())
        {
            project->m_Environment = project->m_AssetManager->Get<EnvironmentAsset>(project->m_Config.EnvironmentPath.string());
        }

        return s_ActiveProject;
    }

    s_ActiveProject = nullptr;
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

    // -------------------------------------------------------------------------------------------------------------------
    // Path Utility Helpers
    // -------------------------------------------------------------------------------------------------------------------

    std::filesystem::path Project::NormalizePath(const std::filesystem::path& path)
    {
        std::filesystem::path normalized = std::filesystem::absolute(path).lexically_normal();
        
        #ifdef CH_PLATFORM_WINDOWS
        // Normalize drive letter to lowercase for consistent comparison
        std::string pathStr = normalized.generic_string();
        std::transform(pathStr.begin(), pathStr.end(), pathStr.begin(), ::tolower);
        normalized = pathStr;
        #endif
        
        return normalized;
    }

    std::optional<std::string> Project::TryMakeRelative(
        const std::filesystem::path& absolutePath,
        const std::filesystem::path& basePath)
    {
        if (basePath.empty())
            return std::nullopt;
        
        auto normalizedBase = NormalizePath(basePath);
        std::filesystem::path rel = std::filesystem::relative(absolutePath, normalizedBase);
        std::string relStr = rel.generic_string();
            
        // Only return if path doesn't escape the base directory
        if (relStr.find("..") == std::string::npos) {
            return relStr;
        }
        
        return std::nullopt;
    }
} // namespace CHEngine
