#ifndef CH_PROJECT_H
#define CH_PROJECT_H

#include "engine/core/base.h"
#include "filesystem"
#include "memory"
#include "string"

namespace CHEngine
{
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

struct ProjectConfig
{
    std::string Name = "Untitled";
    std::string StartScene;
    std::filesystem::path AssetDirectory = "assets";
    std::filesystem::path ProjectDirectory;
    std::filesystem::path ActiveScenePath;

    PhysicsSettings Physics;
    AnimationSettings Animation;
    RenderSettings Render;
    WindowSettings Window;
    RuntimeSettings Runtime;

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

    static std::filesystem::path GetAssetDirectory()
    {
        if (s_ActiveProject)
            return s_ActiveProject->m_Config.ProjectDirectory /
                   s_ActiveProject->m_Config.AssetDirectory;
        return "";
    }

    static std::filesystem::path GetProjectDirectory()
    {
        if (s_ActiveProject)
            return s_ActiveProject->m_Config.ProjectDirectory;
        return "";
    }

    static std::filesystem::path GetAssetPath(const std::filesystem::path &relative)
    {
        return GetAssetDirectory() / relative;
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

private:
    ProjectConfig m_Config;
    static std::shared_ptr<Project> s_ActiveProject;

    friend class ProjectSerializer;
};
} // namespace CHEngine

#endif // CH_PROJECT_H
