#include "action_commands.h"
#include <fstream>

namespace Chained
{

void EditorActionCommands::RenameAsset(const std::filesystem::path& path, const std::string& newName)
{
    std::filesystem::path newPath = path.parent_path() / newName;
    std::error_code ec;
    std::filesystem::rename(path, newPath, ec);
}

void EditorActionCommands::DeleteAsset(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
}

void EditorActionCommands::CreateFolder(const std::filesystem::path& parentPath, const std::string& name)
{
    std::filesystem::path newDir = parentPath / name;
    int i = 1;
    while (std::filesystem::exists(newDir))
    {
        newDir = parentPath / (name + " " + std::to_string(i++));
    }

    std::filesystem::create_directory(newDir);
}

} // namespace Chained
