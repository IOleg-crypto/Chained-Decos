//
// Created by AI Assistant
// PathUtils - Implementation
//

#include "PathUtils.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace PathUtils
{
    std::string ResolveSkyboxAbsolutePath(const std::string& texturePath)
    {
        if (texturePath.empty())
        {
            return "";
        }

        fs::path input(texturePath);
        std::error_code ec;

        if (input.is_absolute())
        {
            fs::path canonical = fs::weakly_canonical(input, ec);
            if (!ec && fs::exists(canonical))
            {
                return canonical.string();
            }
            if (fs::exists(input))
            {
                return input.string();
            }
        }

        fs::path projectRoot(PROJECT_ROOT_DIR);
        fs::path combined = projectRoot / input;
        fs::path canonicalCombined = fs::weakly_canonical(combined, ec);
        if (!ec && fs::exists(canonicalCombined))
        {
            return canonicalCombined.string();
        }
        if (fs::exists(combined))
        {
            return combined.string();
        }

        return (input.is_absolute() ? input : combined).string();
    }

    std::string NormalizePath(const std::string& path, const std::string& basePath)
    {
        if (path.empty())
        {
            return "";
        }

        fs::path base(basePath);
        fs::path input(path);
        std::error_code ec;
        fs::path absolute = input.is_absolute() ? fs::weakly_canonical(input, ec)
                                               : fs::weakly_canonical(base / input, ec);

        if (ec)
        {
            absolute = input.is_absolute() ? input : base / input;
        }

        fs::path relative = fs::relative(absolute, base, ec);
        if (!ec && !relative.empty() && relative.generic_string() != ".")
        {
            return relative.generic_string();
        }

        return absolute.generic_string();
    }
}

