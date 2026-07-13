#include "file_dialogs.h"
#include <nfd.h>

#if defined(__linux__)
#include <cstdlib>
#endif

namespace Chained
{
    void FileDialogs::Init()
    {
#if defined(__linux__)
        // NFD's GTK backend calls gtk_init_check(), which lazily loads the AT-SPI
        // accessibility bridge (atk-bridge) on first use. That bridge allocates
        // process-lifetime state it never frees and spams stderr when no
        // accessibility bus is available (e.g. headless CI) - neither of which we
        // need for a native file/folder picker. Opting out before NFD_Init() avoids
        // both the LeakSanitizer false-positive and the AT-SPI DBus warnings.
        setenv("NO_AT_BRIDGE", "1", 0);
#endif
        NFD_Init();
    }

    void FileDialogs::Shutdown()
    {
        NFD_Quit();
    }

    std::optional<std::filesystem::path> FileDialogs::OpenFile(const std::vector<FileDialogFilter>& filters)
    {
        nfdu8char_t* outPath = nullptr;
        std::vector<nfdu8filteritem_t> nfdFilters;
        for (const auto& filter : filters)
        {
            nfdFilters.push_back({filter.Name.c_str(), filter.Spec.c_str()});
        }

        nfdresult_t result = NFD_OpenDialogU8(&outPath, nfdFilters.data(), (nfdfiltersize_t)nfdFilters.size(), nullptr);
        if (result == NFD_OKAY)
        {
            std::filesystem::path path = outPath;
            NFD_FreePathU8(outPath);
            return path;
        }
        return std::nullopt;
    }

    std::optional<std::filesystem::path> FileDialogs::SaveFile(const std::vector<FileDialogFilter>& filters)
    {
        nfdu8char_t* outPath = nullptr;
        std::vector<nfdu8filteritem_t> nfdFilters;
        for (const auto& filter : filters)
        {
            nfdFilters.push_back({filter.Name.c_str(), filter.Spec.c_str()});
        }

        nfdresult_t result = NFD_SaveDialogU8(&outPath, nfdFilters.data(), (nfdfiltersize_t)nfdFilters.size(), nullptr, NULL);
        if (result == NFD_OKAY)
        {
            std::filesystem::path path = outPath;
            NFD_FreePathU8(outPath);
            return path;
        }
        return std::nullopt;
    }

    std::optional<std::filesystem::path> FileDialogs::PickFolder()
    {
        nfdu8char_t* outPath = nullptr;
        nfdresult_t result = NFD_PickFolderU8(&outPath, nullptr);
        if (result == NFD_OKAY)
        {
            std::filesystem::path path = outPath;
            NFD_FreePathU8(outPath);
            return path;
        }
        return std::nullopt;
    }
}
