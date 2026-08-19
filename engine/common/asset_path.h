#ifndef CH_ASSET_PATH_H
#define CH_ASSET_PATH_H

#include <algorithm>
#include <filesystem>
#include <string>

namespace Chained
{

	inline bool FileExists(const std::filesystem::path& path)
	{
		std::error_code ec;
		return std::filesystem::exists(path, ec);
	}

	inline std::string TrimCopy(const std::string& value)
	{
		auto begin =
			std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
		auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
					   return std::isspace(ch) != 0;
				   }).base();

		if (begin >= end)
		{
			return {};
		}

		return std::string(begin, end);
	}

	inline std::string NormalizeAssetPath(const std::string& path)
	{
		std::string normalized = TrimCopy(path);
		if (normalized.empty())
		{
			return normalized;
		}

		std::replace(normalized.begin(), normalized.end(), '\\', '/');

		while (normalized.rfind("./", 0) == 0)
		{
			normalized = normalized.substr(2);
		}

		if (normalized.rfind("assets/", 0) == 0)
		{
			normalized = normalized.substr(7);
		}

		return normalized;
	}

} // namespace Chained

#endif // CH_ASSET_PATH_H
