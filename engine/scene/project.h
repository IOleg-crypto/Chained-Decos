#ifndef CH_PROJECT_H
#define CH_PROJECT_H

#include "engine/core/base.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/texture_asset.h"
#include <filesystem>
#include <memory>
#include <string>

namespace CHEngine
{
struct LaunchProfile
{
    std::string Name;
    std::string BinaryPath;
    std::string Arguments;
    bool UseDefaultArgs = true;
};

struct PhysicsSettings
{
    float Gravity = 20.0f;
};

struct AnimationSettings
{
    float TargetFPS = 30.0f;
};

struct RenderSettings
{
    float AmbientIntensity = 0.3f;
    float DefaultExposure = 1.0f;
};

struct WindowSettings
{
    int Width = 1280;
    int Height = 720;
    bool VSync = true;
    bool Resizable = true;
};

struct RuntimeSettings
{
    bool Fullscreen = false;
    bool ShowStats = true;
    bool EnableConsole = false;
};

enum class Configuration
{
    Debug = 0,
    Release = 1
};

struct EditorSettings
{
    float CameraMoveSpeed = 10.0f;
    float CameraRotationSpeed = 0.1f;
    float CameraBoostMultiplier = 5.0f;
};

struct ProjectConfig
{
    std::string Name = "Untitled";
    std::string StartScene;
    std::filesystem::path AssetDirectory = "assets";
    std::filesystem::path ProjectDirectory;
    std::filesystem::path ActiveScenePath;
    std::filesystem::path EnvironmentPath;

    PhysicsSettings Physics;
    AnimationSettings Animation;
    RenderSettings Render;
    WindowSettings Window;
    RuntimeSettings Runtime;
    EditorSettings Editor;

    std::vector<LaunchProfile> LaunchProfiles;
    int ActiveLaunchProfileIndex = 0;

    Configuration BuildConfig = Configuration::Debug;
};

class Project
{
public:
    Project() = default;
    ~Project() = default;

    const ProjectConfig &GetConfig() const
    {
        return m_Config;
    }
    ProjectConfig &GetConfig()
    {
        return m_Config;
    }

    static std::shared_ptr<Project> GetActive()
    {
        return s_ActiveProject;
    }

    static void SetActive(std::shared_ptr<Project> project)
    {
        s_ActiveProject = project;
    }

    static std::shared_ptr<Project> New();
    static std::shared_ptr<Project> Load(const std::filesystem::path &path);
    static bool SaveActive(const std::filesystem::path &path);

    static std::vector<std::string> GetAvailableScenes();

    static std::filesystem::path GetAssetDirectory()
    {
        if (s_ActiveProject)
        {
            return s_ActiveProject->m_Config.ProjectDirectory /
                   s_ActiveProject->m_Config.AssetDirectory;
        }
        return "";
    }

    static std::filesystem::path GetProjectDirectory()
    {
        if (s_ActiveProject)
        {
            return s_ActiveProject->m_Config.ProjectDirectory;
        }
        return "";
    }

    static std::filesystem::path GetAssetPath(const std::filesystem::path &relative)
    {
        return GetAssetDirectory() / relative;
    }

    static std::string GetRelativePath(const std::filesystem::path &path)
    {
        if (path.empty()) 
            return "";
        
        if (path.is_relative()) 
            return path.generic_string();

        auto absolutePath = NormalizePath(path);
        std::string finalPath = absolutePath.generic_string();
        
        // Try relative to assets directory first
        if (auto rel = TryMakeRelative(absolutePath, GetAssetDirectory()))
            finalPath = *rel;
        // Fallback to project root
        else if (auto rel = TryMakeRelative(absolutePath, GetProjectDirectory()))
            finalPath = *rel;

        #ifdef CH_PLATFORM_WINDOWS
        std::transform(finalPath.begin(), finalPath.end(), finalPath.begin(), ::tolower);
        std::replace(finalPath.begin(), finalPath.end(), '\\', '/');
        #endif

        return finalPath;
    }

    void SetActiveScenePath(const std::filesystem::path &path)
    {
        m_Config.ActiveScenePath = path;
    }

    void SetName(const std::string &name)
    {
        m_Config.Name = name;
    }

    void SetProjectDirectory(const std::filesystem::path &path)
    {
        m_Config.ProjectDirectory = path;
    }

    void SetEnvironment(const std::filesystem::path &path)
    {
        m_Config.EnvironmentPath = path;
    }

    std::shared_ptr<EnvironmentAsset> GetEnvironment() const
    {
        return m_Environment;
    }

    std::shared_ptr<AssetManager> GetAssetManager() const
    {
        return m_AssetManager;
    }

private:
    ProjectConfig m_Config;
    std::shared_ptr<EnvironmentAsset> m_Environment;
    std::shared_ptr<AssetManager> m_AssetManager;
    static std::shared_ptr<Project> s_ActiveProject;
    
    // Path utility helpers
    static std::filesystem::path NormalizePath(const std::filesystem::path& path);
    static std::optional<std::string> TryMakeRelative(
        const std::filesystem::path& absolutePath,
        const std::filesystem::path& basePath);

    friend class ProjectSerializer;
};
} // namespace CHEngine

#endif // CH_PROJECT_H
