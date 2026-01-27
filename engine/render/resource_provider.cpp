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
        std::filesystem::path projectDir = Project::GetProjectDirectory();

        // 1. Clean the path a bit
        std::string pathStr = p.string();
        std::filesystem::path cleanPath = p;

        // Strip redundant "assets/" or "game/..." if accidentally passed
        if (pathStr.starts_with("assets/") || pathStr.starts_with("assets\\"))
            cleanPath = pathStr.substr(7);

        // List of candidates to check
        std::vector<std::filesystem::path> candidates;

        // Path as provided (relative to assets)
        candidates.push_back(assetDir / cleanPath);

        // Extension-based smart search
        std::string ext = cleanPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".chscene")
        {
            candidates.push_back(assetDir / "scenes" / cleanPath);
        }
        else if (ext == ".chproject")
        {
            candidates.push_back(projectDir / cleanPath);
        }
        else if (ext == ".lua" || ext == ".cs")
        {
            candidates.push_back(assetDir / "scripts" / cleanPath);
        }
        else if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb")
        {
            candidates.push_back(assetDir / "Models" / cleanPath);
        }
        else if (ext == ".png" || ext == ".jpg" || ext == ".tga" || ext == ".bmp")
        {
            candidates.push_back(assetDir / "textures" / cleanPath);
            candidates.push_back(assetDir / "materials" / cleanPath);
        }

        // Check if any candidate exists
        for (auto &cand : candidates)
        {
            auto preferred = cand.lexically_normal().make_preferred();
            if (std::filesystem::exists(preferred))
                return preferred;
        }

        // Fallback for new files
        return (assetDir / cleanPath).make_preferred();
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
