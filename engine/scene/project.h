#ifndef CH_PROJECT_H
#define CH_PROJECT_H

#include "engine/core/base.h"
#include <filesystem>
#include <string>

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

    static Ref<Project> GetActive()
    {
        return s_ActiveProject;
    }
    static void SetActive(Ref<Project> project)
    {
        s_ActiveProject = project;
    }

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
    static Ref<Project> s_ActiveProject;

    friend class ProjectSerializer;
};
} // namespace CHEngine

#endif // CH_PROJECT_H
