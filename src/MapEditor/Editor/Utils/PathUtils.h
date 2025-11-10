#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <string>

// Utility namespace for path operations
namespace PathUtils
{
    // Resolve absolute path for skybox texture
    // Handles both relative (from JSON files) and absolute paths (from NFD)
    // NFD returns absolute paths, so this mainly helps with relative paths from saved maps
    std::string ResolveSkyboxAbsolutePath(const std::string& texturePath);

    // General path normalization (can be extended for other path types)
    std::string NormalizePath(const std::string& path, const std::string& basePath = PROJECT_ROOT_DIR);
}

#endif // PATH_UTILS_H

