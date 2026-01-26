#include "resource_provider.h"
#include "engine/scene/project.h"
#include <filesystem>

namespace CHEngine
{
static std::filesystem::path ResolveEnginePath(const std::string &path)
{
    if (path.starts_with("engine:"))
        return (std::filesystem::path(PROJECT_ROOT_DIR) / "engine/resources" / path.substr(7))
            .make_preferred();
    return "";
}

static std::filesystem::path ResolveProjectPath(const std::filesystem::path &p)
{
    if (Project::GetActive())
    {
        std::filesystem::path assetDir = Project::GetAssetDirectory();

        // 1. Try relative to asset directory
        std::filesystem::path fullPath = (assetDir / p).make_preferred();
        if (std::filesystem::exists(fullPath))
            return fullPath;

        // 2. Try stripping "assets/"
        std::string pathStr = p.string();
        if (pathStr.starts_with("assets/") || pathStr.starts_with("assets\\"))
        {
            std::filesystem::path stripped = pathStr.substr(7);
            fullPath = (assetDir / stripped).make_preferred();
            if (std::filesystem::exists(fullPath))
                return fullPath;

            // 3. Try relative to project root directly
            fullPath = (Project::GetProjectDirectory() / p).make_preferred();
            if (std::filesystem::exists(fullPath))
                return fullPath;
        }

        // Default to asset directory for new files
        return (assetDir / p).make_preferred();
    }
    return p;
}

std::filesystem::path ResourceProvider::ResolvePath(const std::string &path)
{
    if (path.empty())
        return "";

    auto enginePath = ResolveEnginePath(path);
    if (!enginePath.empty())
        return enginePath;

    std::filesystem::path p = path;
    if (p.is_absolute())
        return p.make_preferred();

    return ResolveProjectPath(p).make_preferred();
}

bool ResourceProvider::Exists(const std::string &path)
{
    return std::filesystem::exists(ResolvePath(path));
}
} // namespace CHEngine
