#ifndef CH_PROJECT_H
#define CH_PROJECT_H

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace Chained
{
class EnvironmentAsset;

struct PhysicsSettings
{
    float Gravity = 20.0f;
    float FixedTimestep = 1.0f / 60.0f;
};

struct AnimationSettings
{
    float TargetFPS = 30.0f;
};

struct RenderSettings
{
    int ShadowResolution = 2048;
    bool EnableShadows = true;
    int AntiAliasingSamples = 4; // 0, 2, 4, 8
};

struct MeshSettings
{
    bool ImportMaterials = true;
    bool CalculateTangents = true;
    bool FlipUVs = true;
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
    int TargetFPS = 0; // 0 = Uncapped
};

struct AudioSettings
{
    float MasterVolume = 1.0f;
    float MusicVolume = 1.0f;
    float SFXVolume = 1.0f;
};

struct ScriptingSettings
{
    std::string ModuleName;
    std::filesystem::path ModuleDirectory;
    bool AutoLoad = true;
};

enum class Configuration
{
    Debug = 0,
    Release = 1
};



struct ProjectConfig
{
    std::string Name = "Untitled";
    std::string IconPath;
    std::string StartScene;
    std::filesystem::path AssetDirectory = "assets";
    std::filesystem::path ProjectDirectory;
    std::filesystem::path ActiveScenePath;
    std::filesystem::path EnvironmentPath;

    std::vector<std::string> BuildScenes; // List of scenes included in build

    PhysicsSettings Physics;
    AnimationSettings Animation;
    RenderSettings Render;
    MeshSettings Mesh;
    WindowSettings Window;
    AudioSettings Audio;
    RuntimeSettings Runtime;
    ScriptingSettings Scripting;

    Configuration BuildConfig = Configuration::Debug;
};

/// @brief Owns the active project configuration and environment asset, plus path helpers
/// rooted at the process-wide active project.
///
/// Projects define the game's settings, asset directories, and scene list.
/// A single project is active at any time (set via SetActive). Static methods
/// operate on the active project; instance methods operate on a specific project.
class Project
{
public:
    Project() = default;
    ~Project();

    /// @brief Load a project from a .chproject YAML file.
    /// @param filepath Path to the .chproject file.
    /// @return The loaded project, or nullptr on failure.
    static std::shared_ptr<Project> Load(const std::filesystem::path& filepath);

    /// @brief Get the currently active project.
    static std::shared_ptr<Project> GetActive();

    /// @brief Set the active project (called by RuntimeLayer or Editor after loading).
    static void SetActive(std::shared_ptr<Project> project);

    // Returns the active project configuration.
    const ProjectConfig& GetConfig() const { return m_Config; }
    // Returns the active project configuration for mutation.
    ProjectConfig& GetConfig() { return m_Config; }

    // Path helpers (now non-static, relative to this project)
    std::filesystem::path GetAssetDirectoryForProject() const {
        return m_Config.ProjectDirectory / m_Config.AssetDirectory;
    }

    static std::filesystem::path GetAssetDirectory();

    std::filesystem::path GetProjectDirectoryForProject() const {
        return m_Config.ProjectDirectory;
    }

    static std::filesystem::path GetProjectDirectory();

    std::filesystem::path GetAssetPathForProject(const std::filesystem::path& relative) const { return GetAssetDirectoryForProject() / relative; }

    static std::filesystem::path GetAssetPath(const std::filesystem::path& relative);

    // Converts a path to a project-relative string when possible.
    // NOTE: For EngineRoot resolution, use ProjectManager.
    std::string GetRelativePathForProject(const std::filesystem::path& path) const;
    static std::string GetRelativePath(const std::filesystem::path& path);
    // Converts a path to an absolute path under this project.
    std::filesystem::path GetAbsolutePathForProject(const std::filesystem::path& path) const;
    static std::filesystem::path GetAbsolutePath(const std::filesystem::path& path);
    std::vector<std::string> GetAvailableScenes() const;

    // static utility (Pure functions)
    static std::filesystem::path NormalizePath(const std::filesystem::path& path);
    static std::optional<std::string> TryMakeRelative(const std::filesystem::path& absolutePath,
                                                                    const std::filesystem::path& basePath);

    // Environment
    std::shared_ptr<EnvironmentAsset> GetEnvironment() const { return m_Environment; }


private:
    static std::shared_ptr<Project> s_ActiveProject;

    ProjectConfig m_Config;
    std::shared_ptr<EnvironmentAsset> m_Environment;


    friend class ProjectSerializer;
};
} // namespace Chained

#endif // CH_PROJECT_H
