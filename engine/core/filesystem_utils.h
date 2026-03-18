#ifndef CH_FILESYSTEM_UTILS_H
#define CH_FILESYSTEM_UTILS_H

#include <filesystem>

namespace CHEngine {

class FilesystemUtils
{
public:
    static std::filesystem::path GetExecutableDirectory();
};

} // namespace CHEngine

#endif // CH_FILESYSTEM_UTILS_H
