#ifndef CH_PLATFORM_H
#define CH_PLATFORM_H

#include "engine/foundation/base.h"
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace Chained
{
    struct CH_API FileDialogFilter
    {
        std::string Name;
        std::string Spec;
    };

    class CH_API Platform
    {
    public:
        // Returns the absolute path to the directory containing the current executable.
        static std::filesystem::path GetExecutableDirectory();

        // Returns the current system time in seconds.
        static float GetTime();

        // Opens a file dialog and returns the selected path.
        static std::optional<std::filesystem::path> OpenFile(const std::vector<FileDialogFilter>& filters = {});
        
        // Opens a save file dialog and returns the selected path.
        static std::optional<std::filesystem::path> SaveFile(const std::vector<FileDialogFilter>& filters = {});

        // Opens a folder picker dialog and returns the selected path.
        static std::optional<std::filesystem::path> PickFolder();

        // Sleeps the current thread for the specified milliseconds.
        static void Sleep(uint32_t milliseconds);
    };
}

#endif // CH_PLATFORM_H
