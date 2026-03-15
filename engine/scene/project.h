#ifndef CH_PROJECT_H
#define CH_PROJECT_H

#include "engine/core/base.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/environment.h"
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
    bool          GenerateMipmaps = true;
    TextureFilter Filter          = TextureFilter::Bilinear;
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

class Project
{
public:
    Project() = default;
    ~Project() = default;

    [[nodiscard]] const ProjectConfig& GetConfig() const
    {
        return m_Config;
    }
    [[nodiscard]] ProjectConfig& GetConfig()
    {
        return m_Config;
    }

    [[nodiscard]] static std::shared_ptr<Project> GetActive()
    {
        return s_ActiveProject;
    }

    static void SetActive(std::shared_ptr<Project> project)
    {
        s_ActiveProject = project;
    }

    [[nodiscard]] static std::shared_ptr<Project> New();
    [[nodiscard]] static std::shared_ptr<Project> Load(const std::filesystem::path& path);
    [[nodiscard]] static std::filesystem::path Discover(const std::filesystem::path& startPath = "",
                                                         const std::string& hintName = "");

    [[nodiscard]] static std::filesystem::path GetEngineRoot()
    {
        return s_EngineRoot;
    }
    static void SetEngineRoot(const std::filesystem::path& path)
    {
        s_EngineRoot = path;
    }

    static bool SaveActive(const std::filesystem::path& path);

    [[nodiscard]] static std::vector<std::string> GetAvailableScenes();

    [[nodiscard]] static std::filesystem::path GetAssetDirectory()
    {
        if (s_ActiveProject)
        {
            return s_ActiveProject->m_Config.ProjectDirectory / s_ActiveProject->m_Config.AssetDirectory;
        }
        return "";
    }

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

    [[nodiscard]] static std::string GetRelativePath(const std::filesystem::path& path);

    // Path utility helpers
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
