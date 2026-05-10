#ifndef CH_PROJECT_H
#define CH_PROJECT_H

#include "engine/core/base.h"
#include "engine/graphics/assets/environment.h"
#include "engine/graphics/assets/texture_asset.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace CHEngine
{
// Describes an executable launch profile for a project build.
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
    float FixedTimestep = 1.0f / 60.0f;
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

struct MeshSettings
{
    bool ImportMaterials = true;
    bool CalculateTangents = true;
    bool FlipUVs = true;
};

// 0=None, 1=Bilinear, 2=Trilinear, 3=Anisotropic 4x, 4=Anisotropic 8x, 5=Anisotropic 16x
enum class TextureFilter : int
{
    None        = 0,
    Bilinear    = 1,
    Trilinear   = 2,
    Anisotropic4x  = 3,
    Anisotropic8x  = 4,
    Anisotropic16x = 5
};

struct TextureSettings
{
    bool GenerateMipmaps = true;
    TextureFilter Filter = TextureFilter::Bilinear;
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

struct EditorSettings
{
    float CameraMoveSpeed = 10.0f;
    float CameraRotationSpeed = 0.1f;
    float CameraBoostMultiplier = 5.0f;
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
    TextureSettings Texture;
    MeshSettings Mesh;
    WindowSettings Window;
    RuntimeSettings Runtime;
    EditorSettings Editor;
    ScriptingSettings Scripting;

    std::vector<LaunchProfile> LaunchProfiles;
    int ActiveLaunchProfileIndex = 0;

    Configuration BuildConfig = Configuration::Debug;
};

// Owns the active project configuration and environment asset, plus path helpers
// rooted at the process-wide active project.
class Project
{
public:
    Project() = default;
    ~Project() = default;

    // Returns the active project configuration.
    [[nodiscard]] const ProjectConfig& GetConfig() const
    {
        return m_Config;
    }
    // Returns the active project configuration for mutation.
    [[nodiscard]] ProjectConfig& GetConfig()
    {
        return m_Config;
    }

    // Returns the process-wide active project, or null if none is loaded.
    [[nodiscard]] static std::shared_ptr<Project> GetActive()
    {
        return s_ActiveProject;
    }

    // Sets the process-wide active project.
    static void SetActive(std::shared_ptr<Project> project);

    // Creates a new in-memory project with default settings.
    [[nodiscard]] static std::shared_ptr<Project> New();
    // Loads a project from disk.
    [[nodiscard]] static std::shared_ptr<Project> Load(const std::filesystem::path& path);
    // Discovers a project file by walking from a starting directory.
    [[nodiscard]] static std::filesystem::path Discover(const std::filesystem::path& startPath = "",
                                                         const std::string& hintName = "");

    // Returns the engine root used for resolving engine-relative paths.
    [[nodiscard]] static std::filesystem::path GetEngineRoot()
    {
        return s_EngineRoot;
    }
    // Sets the engine root used for resolving engine-relative paths.
    static void SetEngineRoot(const std::filesystem::path& path);
    // Discovers the engine root by looking for the "resources" folder from the given path.
    static void DiscoverEngineRoot(const std::filesystem::path& startPath = "");

    // Saves the active project to disk.
    static bool SaveActive(const std::filesystem::path& path);

    // Returns the scenes that are available to the active project.
    [[nodiscard]] static std::vector<std::string> GetAvailableScenes();

    // Returns the active project's asset directory.
    [[nodiscard]] static std::filesystem::path GetAssetDirectory()
    {
        if (s_ActiveProject)
        {
            return s_ActiveProject->m_Config.ProjectDirectory / s_ActiveProject->m_Config.AssetDirectory;
        }
        return "";
    }

    // Returns the active project's directory.
    [[nodiscard]] static std::filesystem::path GetProjectDirectory()
    {
        if (s_ActiveProject)
        {
            return s_ActiveProject->m_Config.ProjectDirectory;
        }
        return "";
    }

    [[nodiscard]] static std::filesystem::path GetAssetPath(const std::filesystem::path& relative)
    {
        return GetAssetDirectory() / relative;
    }

    // Converts a path to a project-relative string when possible.
    [[nodiscard]] static std::string GetRelativePath(const std::filesystem::path& path);
    // Converts a path to an absolute path under the active project or engine root.
    [[nodiscard]] static std::filesystem::path GetAbsolutePath(const std::filesystem::path& path);

    // Path utility helpers.
    [[nodiscard]] static std::filesystem::path NormalizePath(const std::filesystem::path& path);
    [[nodiscard]] static std::optional<std::string> TryMakeRelative(const std::filesystem::path& absolutePath,
                                                                    const std::filesystem::path& basePath);

    void SetActiveScenePath(const std::filesystem::path& path)
    {
        m_Config.ActiveScenePath = path;
    }

    void SetName(const std::string& name)
    {
        m_Config.Name = name;
    }

    void SetProjectDirectory(const std::filesystem::path& path)
    {
        m_Config.ProjectDirectory = path;
    }

    void SetEnvironment(const std::filesystem::path& path)
    {
        m_Config.EnvironmentPath = path;
    }

    // Returns the environment asset associated with this project, if any.
    std::shared_ptr<EnvironmentAsset> GetEnvironment() const
    {
        return m_Environment;
    }


private:
    ProjectConfig m_Config;
    std::shared_ptr<EnvironmentAsset> m_Environment;
    static std::shared_ptr<Project> s_ActiveProject;
    static std::filesystem::path s_EngineRoot;

    friend class ProjectSerializer;
};
} // namespace CHEngine

#endif // CH_PROJECT_H
