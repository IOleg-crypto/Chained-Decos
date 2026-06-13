#include "file_dialogs.h"
#include <nfd.h>

namespace Chained
{
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
