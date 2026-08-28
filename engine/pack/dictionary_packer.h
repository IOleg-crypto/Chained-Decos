#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Chained
{
	struct DictionaryPackItem
	{
		std::filesystem::path FilePath;
		std::string ItemPath;
	};

	using DictionaryPackProgressCallback =
		std::function<void(uint64_t current, uint64_t total, const std::string& itemPath)>;

	class DictionaryPacker
	{
	public:
		static constexpr const char* DictionaryItemPath = "__zstd_dictionary__";

		static bool Pack(const std::filesystem::path& packPath, const std::vector<DictionaryPackItem>& items,
						 uint32_t dataVersion = 0, float zipThreshold = 0.0f, bool printProgress = false,
						 const DictionaryPackProgressCallback& onProgress = nullptr, int compressionLevel = 9);
	};
} // namespace Chained
