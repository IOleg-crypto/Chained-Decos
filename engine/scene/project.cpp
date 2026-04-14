#include "project.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/environment.h"
#include "engine/graphics/pipeline/renderer.h"
#include "imgui.h"
#include "project_serializer.h"


namespace CHEngine
{
std::shared_ptr<Project> Project::s_ActiveProject = nullptr;
std::filesystem::path Project::s_EngineRoot = "";

std::shared_ptr<Project> Project::New()
{
    auto project = std::make_shared<Project>();
    s_ActiveProject = project;
    return s_ActiveProject;
}

std::shared_ptr<Project> Project::Load(const std::filesystem::path& path)
{
    std::shared_ptr<Project> project = std::make_shared<Project>();

    project->m_Config.ProjectDirectory = path.parent_path();
    s_ActiveProject = project;

    // Discover Engine Root if not set or invalid
    if (s_EngineRoot.empty() || !std::filesystem::exists(s_EngineRoot / "resources"))
    {
        // 1. Try development root macro
#ifdef PROJECT_ROOT_DIR
        if (std::filesystem::exists(std::filesystem::path(PROJECT_ROOT_DIR) / "resources"))
        {
            s_EngineRoot = PROJECT_ROOT_DIR;
        }
#endif

        // 2. Try traversing up from project file
        if (s_EngineRoot.empty())
        {
            std::filesystem::path current = path.parent_path();
            while (current.has_parent_path())
            {
                if (std::filesystem::exists(current / "resources"))
                {
                    s_EngineRoot = current;
                    break;
                }
                current = current.parent_path();
            }
        }
    }

    ProjectSerializer serializer(project);
    if (serializer.Deserialize(path))
    {
        // Load engine shaders now that paths are set correctly
        if (Renderer::IsInitialized())
        {
            Renderer::LoadEngineResources();
        }

        // Load environment if specified
        if (!project->m_Config.EnvironmentPath.empty())
        {
            project->m_Environment =
                AssetManager::Get().Get<EnvironmentAsset>(project->m_Config.EnvironmentPath.string());
        }

        // --- Automated Shader Discovery ---
        if (Renderer::IsInitialized())
        {
            auto shaderDir = project->m_Config.ProjectDirectory / project->m_Config.AssetDirectory / "shaders";
            if (std::filesystem::exists(shaderDir))
            {
                auto& lib = Renderer::Get().GetShaderLibrary();
                for (const auto& entry : std::filesystem::recursive_directory_iterator(shaderDir))
                {
                    if (entry.path().extension() == ".chshader")
                    {
                        std::string name = entry.path().stem().string();
                        std::string relPath = project->GetRelativePath(entry.path());

                        if (!lib.Exists(name))
                        {
                            lib.Load(name, relPath);
                            CH_CORE_INFO("Project: Discovered and loaded shader: {} ({})", name, relPath);
                        }
                    }
                }
            }
        }

        return s_ActiveProject;
    }

    s_ActiveProject = nullptr;
    return nullptr;
}

std::filesystem::path Project::Discover(const std::filesystem::path& startPath, const std::string& hintName)
{
    std::filesystem::path current = startPath.empty() ? std::filesystem::current_path() : startPath;
    if (std::filesystem::is_regular_file(current))
    {
        current = current.parent_path();
    }

    CH_CORE_INFO("Project: Discovering project starting from: {} (Hint: {})", current.string(), hintName);

    while (true)
    {
        CH_CORE_INFO("Project: Checking directory: {}", current.string());
        std::error_code ec;
        if (std::filesystem::exists(current, ec))
        {
            // 1. Check for {hintName}.chproject (Priority 1)
            if (!hintName.empty())
            {
                std::filesystem::path hintPath = current / (hintName + ".chproject");
                if (std::filesystem::exists(hintPath, ec))
                {
                    CH_CORE_INFO("Project: Found hinted project: {}", hintPath.string());
                    return hintPath;
                }
            }

            // 2. Check for any .chproject (Priority 2)
            for (const auto& entry : std::filesystem::directory_iterator(current, ec))
            {
                if (entry.path().extension() == ".chproject")
                {
                    CH_CORE_INFO("Project: Found project: {}", entry.path().string());
                    return entry.path();
                }
            }

            // 3. Check "game" subdirectory for nested structures
            std::filesystem::path gameDir = current / "game";
            if (std::filesystem::exists(gameDir, ec))
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(gameDir, ec))
                {
                    const auto& p = entry.path();
                    if (p.extension() == ".chproject")
                    {
                        // Even here, prioritize hintName if found
                        if (!hintName.empty() && p.stem().string() == hintName)
                        {
                            CH_CORE_INFO("Project: Found hinted project in game dir: {}", p.string());
                            return p;
                        }
                    }
                }

                // If nested search didn't find the hint, return the first found .chproject in gameDir
                for (const auto& entry : std::filesystem::recursive_directory_iterator(gameDir, ec))
                {
                    if (entry.path().extension() == ".chproject")
                    {
                        return entry.path();
                    }
                }
            }
        }

        if (!current.has_parent_path() || current == current.root_path())
        {
            break;
        }
        current = current.parent_path();
    }

    return "";
}

bool Project::SaveActive(const std::filesystem::path& path)
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
    {
        return scenes;
    }

    auto assetDir = GetAssetDirectory();
    auto scenesDir = assetDir / "scenes";

    if (std::filesystem::exists(scenesDir))
    {
        for (auto& entry : std::filesystem::recursive_directory_iterator(scenesDir))
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

std::string Project::GetRelativePath(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return "";
    }

    if (path.is_relative())
    {
        return path.generic_string();
    }

    auto absolutePath = NormalizePath(path);

    // 1. Try relative to Assets Directory
    if (auto rel = TryMakeRelative(absolutePath, GetAssetDirectory()))
    {
        return *rel;
    }

    // 2. Try relative to Project Root
    if (auto rel = TryMakeRelative(absolutePath, GetProjectDirectory()))
    {
        return *rel;
    }

    // 3. Try relative to Engine Root
    if (!s_EngineRoot.empty())
    {
        if (auto rel = TryMakeRelative(absolutePath, s_EngineRoot))
        {
            return "engine/" + *rel;
        }
    }

    return absolutePath.generic_string();
}

std::filesystem::path Project::GetAbsolutePath(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return "";
    }

    if (path.is_absolute())
    {
        return NormalizePath(path);
    }

    std::string pathStr = path.generic_string();

    // Handle "engine/" prefix (for engine resources)
    bool isEngineResource = false;
    if (pathStr.find("engine/") == 0)
    {
        pathStr = pathStr.substr(7); // Remove "engine/" prefix
        isEngineResource = true;
    }

    if (isEngineResource)
    {
        // Look in engine root first
        if (!s_EngineRoot.empty())
        {
            std::filesystem::path candidate = s_EngineRoot / pathStr;
            if (std::filesystem::exists(candidate))
            {
                return NormalizePath(candidate);
            }
        }
    }
    else
    {
        // For game assets, try asset directory first
        std::filesystem::path assetDir = GetAssetDirectory();
        if (!assetDir.empty())
        {
            std::filesystem::path candidate = assetDir / pathStr;
            if (std::filesystem::exists(candidate))
            {
                return NormalizePath(candidate);
            }
        }

        // Try project root next
        std::filesystem::path projectDir = GetProjectDirectory();
        if (!projectDir.empty())
        {
            std::filesystem::path candidate = projectDir / pathStr;
            if (std::filesystem::exists(candidate))
            {
                return NormalizePath(candidate);
            }
        }
    }

    // Final fallback: return best guess without verifying existence
    // This allows AssetManager async loading to handle missing files gracefully
    std::filesystem::path assetDir = GetAssetDirectory();
    if (!assetDir.empty() && !isEngineResource)
    {
        return NormalizePath(assetDir / pathStr);
    }
    else if (!s_EngineRoot.empty())
    {
        return NormalizePath(s_EngineRoot / pathStr);
    }
    else
    {
        return NormalizePath(GetProjectDirectory() / pathStr);
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Path Utility Helpers
// -------------------------------------------------------------------------------------------------------------------

std::filesystem::path Project::NormalizePath(const std::filesystem::path& path)
{
    // Use lexically_normal to handle .. and . and unify slashes
    std::filesystem::path normalized = std::filesystem::absolute(path).lexically_normal();

#if CH_PLATFORM_WINDOWS
    // Unify drive letter casing to uppercase to prevent relative path resolution failures
    std::string s = normalized.string();
    if (s.length() >= 2 && s[1] == ':' && std::islower(s[0]))
    {
        s[0] = (char)std::toupper(s[0]);
        normalized = s;
    }
#endif

    // On Windows, generic_string() will use / which is exactly what we want for cross-platform portability.
    return normalized;
}

std::optional<std::string> Project::TryMakeRelative(const std::filesystem::path& absolutePath,
                                                    const std::filesystem::path& basePath)
{
    if (basePath.empty())
    {
        return std::nullopt;
    }

    auto normalizedBase = NormalizePath(basePath);
    std::filesystem::path rel = std::filesystem::relative(absolutePath, normalizedBase);
    
    // std::filesystem::relative returns an absolute path if it cannot resolve relativity (e.g., different drives)
    if (rel.is_relative())
    {
        std::string relStr = rel.generic_string();

        // Only return if path doesn't escape the base directory
        if (relStr.find("..") == std::string::npos)
        {
            return relStr;
        }
    }

    return std::nullopt;
}
} // namespace CHEngine
