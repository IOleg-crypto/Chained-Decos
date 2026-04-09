#include "dialogs.h"
#include <nfd.h>

namespace CHEngine
{
std::optional<std::filesystem::path> Dialogs::OpenFile(const std::vector<FileDialogFilter>& filters)
{
    nfdu8char_t* outPath = NULL;
    std::vector<nfdu8filteritem_t> nfdFilters;
    for (const auto& filter : filters)
    {
        nfdFilters.push_back({filter.Name.c_str(), filter.Spec.c_str()});
    }

    nfdresult_t result = NFD_OpenDialogU8(&outPath, nfdFilters.data(), (nfdfiltersize_t)nfdFilters.size(), NULL);
    if (result == NFD_OKAY)
    {
        std::filesystem::path path = outPath;
        NFD_FreePathU8(outPath);
        return path;
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> Dialogs::SaveFile(const std::vector<FileDialogFilter>& filters)
{
    nfdu8char_t* outPath = NULL;
    std::vector<nfdu8filteritem_t> nfdFilters;
    for (const auto& filter : filters)
    {
        nfdFilters.push_back({filter.Name.c_str(), filter.Spec.c_str()});
    }

    nfdresult_t result = NFD_SaveDialogU8(&outPath, nfdFilters.data(), (nfdfiltersize_t)nfdFilters.size(), NULL, NULL);
    if (result == NFD_OKAY)
    {
        std::filesystem::path path = outPath;
        NFD_FreePathU8(outPath);
        return path;
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> Dialogs::PickFolder()
{
    nfdu8char_t* outPath = NULL;
    nfdresult_t result = NFD_PickFolderU8(&outPath, NULL);
    if (result == NFD_OKAY)
    {
        std::filesystem::path path = outPath;
        NFD_FreePathU8(outPath);
        return path;
    }
    return std::nullopt;
}
} // namespace CHEngine
