#ifndef CH_DIALOGS_H
#define CH_DIALOGS_H

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace CHEngine
{
struct FileDialogFilter
{
    std::string Name;
    std::string Spec;
};

class Dialogs
{
public:
    static std::optional<std::filesystem::path> OpenFile(const std::vector<FileDialogFilter>& filters = {});
    static std::optional<std::filesystem::path> SaveFile(const std::vector<FileDialogFilter>& filters = {});
    static std::optional<std::filesystem::path> PickFolder();
};
} // namespace CHEngine

#endif // CH_DIALOGS_H
