#ifndef CH_FILE_DIALOGS_H
#define CH_FILE_DIALOGS_H

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

    class CH_API FileDialogs
    {
    public:
        static void Init();
        static void Shutdown();

        // Opens a file dialog and returns the selected path.
        static std::optional<std::filesystem::path> OpenFile(const std::vector<FileDialogFilter>& filters = {});
        
        // Opens a save file dialog and returns the selected path.
        static std::optional<std::filesystem::path> SaveFile(const std::vector<FileDialogFilter>& filters = {});

        // Opens a folder picker dialog and returns the selected path.
        static std::optional<std::filesystem::path> PickFolder();
    };
}

#endif // CH_FILE_DIALOGS_H
