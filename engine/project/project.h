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
    float AmbientIntensity = 0.3f;
    float DefaultExposure = 1.0f;
    int ShadowResolution = 2048;
    bool EnableSSAO = false;
    bool EnableBloom = true;
    int AntiAliasingSamples = 4; // 0, 2, 4, 8
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
    TextureSettings Texture;
    MeshSettings Mesh;
    WindowSettings Window;
    AudioSettings Audio;
    RuntimeSettings Runtime;
    ScriptingSettings Scripting;

    Configuration BuildConfig = Configuration::Debug;
};

// Owns the active project configuration and environment asset, plus path helpers
// rooted at the process-wide active project.
class Project
{
public:
    Project() = default;
    ~Project();

    static std::shared_ptr<Project> Load(const std::filesystem::path& filepath);
    static std::shared_ptr<Project> GetActive();
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
