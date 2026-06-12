#ifndef CH_EDITOR_ACTION_COMMANDS_H
#define CH_EDITOR_ACTION_COMMANDS_H

#include <filesystem>
#include <string>

namespace Chained
{

namespace EditorActionCommands
{
void RenameAsset(const std::filesystem::path& path, const std::string& newName);
void DeleteAsset(const std::filesystem::path& path);
void CreateFolder(const std::filesystem::path& parentPath, const std::string& name = "New Folder");
}; // namespace EditorActionCommands

} // namespace Chained

#endif // CH_EDITOR_ACTION_COMMANDS_H
