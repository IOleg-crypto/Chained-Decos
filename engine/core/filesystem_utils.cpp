#include "filesystem_utils.h"

#ifdef CH_PLATFORM_WINDOWS
    #define Rectangle _Rectangle
    #define CloseWindow _CloseWindow
    #define ShowCursor _ShowCursor
    #include <windows.h>
    #undef Rectangle
    #undef CloseWindow
    #undef ShowCursor
#elif defined(CH_PLATFORM_LINUX)
    #include <unistd.h>
#endif

namespace CHEngine {

std::filesystem::path FilesystemUtils::GetExecutableDirectory()
{
#ifdef CH_PLATFORM_WINDOWS
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
#elif defined(CH_PLATFORM_LINUX)
    char path[1024];
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path));
    if (count != -1)
        return std::filesystem::path(std::string(path, count)).parent_path();
#endif
    return std::filesystem::current_path();
}

} // namespace CHEngine
