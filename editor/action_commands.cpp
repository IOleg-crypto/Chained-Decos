#include "action_commands.h"
#include "engine/core/log.h"
#include <fstream>

namespace Chained
{

	void EditorActionCommands::RenameAsset(const std::filesystem::path& path, const std::string& newName)
	{
		if (!std::filesystem::exists(path))
		{
			CH_CORE_WARN("RenameAsset: source does not exist: {}", path.string());
			return;
		}

		std::filesystem::path newPath = path.parent_path() / newName;
		if (std::filesystem::exists(newPath))
		{
			CH_CORE_ERROR("RenameAsset: destination already exists: {}", newPath.string());
			return;
		}

		std::error_code ec;
		std::filesystem::rename(path, newPath, ec);
		if (ec)
		{
			CH_CORE_ERROR("RenameAsset: failed to rename '{}' -> '{}': {}", path.string(), newPath.string(),
						  ec.message());
		}
	}

	void EditorActionCommands::DeleteAsset(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			CH_CORE_WARN("DeleteAsset: path does not exist: {}", path.string());
			return;
		}

		std::error_code ec;
		std::filesystem::remove_all(path, ec);
		if (ec)
		{
			CH_CORE_ERROR("DeleteAsset: failed to delete '{}': {}", path.string(), ec.message());
		}
	}

	void EditorActionCommands::CreateFolder(const std::filesystem::path& parentPath, const std::string& name)
	{
		if (!std::filesystem::exists(parentPath))
		{
			CH_CORE_ERROR("CreateFolder: parent does not exist: {}", parentPath.string());
			return;
		}

		std::filesystem::path newDir = parentPath / name;
		if (std::filesystem::exists(newDir))
		{
			int i = 1;
			constexpr int kMaxAttempts = 1000;
			do
			{
				newDir = parentPath / (name + " " + std::to_string(i++));
				if (i > kMaxAttempts)
				{
					CH_CORE_ERROR("CreateFolder: too many conflicts for '{}' in {}", name, parentPath.string());
					return;
				}
			} while (std::filesystem::exists(newDir));
		}

		std::error_code ec;
		std::filesystem::create_directory(newDir, ec);
		if (ec)
		{
			CH_CORE_ERROR("CreateFolder: failed to create '{}': {}", newDir.string(), ec.message());
		}
	}

} // namespace Chained
