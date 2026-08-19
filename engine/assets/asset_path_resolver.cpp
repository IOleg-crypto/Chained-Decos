#include "engine/assets/asset_path_resolver.h"

namespace Chained
{

	std::string AssetPathResolver::ResolvePackKey(const std::string& assetPath) const
	{
		auto NormalizeSlashes = [](std::string& s) { std::replace(s.begin(), s.end(), '\\', '/'); };

		auto StripPrefix = [](std::string& input, const std::filesystem::path& base) {
			if (base.empty())
			{
				return;
			}
			std::string basePath = base.generic_string();
			if (!basePath.empty() && basePath.back() != '/')
			{
				basePath += '/';
			}
			if (input.size() >= basePath.size() && input.compare(0, basePath.size(), basePath) == 0)
			{
				input = input.substr(basePath.size());
			}
		};

		std::string packKey = assetPath;
		NormalizeSlashes(packKey);
		StripPrefix(packKey, m_EngineRoot);
		StripPrefix(packKey, m_ProjectDirectory);

		packKey = std::filesystem::path(packKey).lexically_normal().generic_string();
		return packKey;
	}

	std::string AssetPathResolver::ResolvePath(const std::string& path) const
	{
		if (path.empty())
		{
			return "";
		}

		{
			std::lock_guard<std::mutex> lock(m_PathMutex);
			if (auto it = m_PathCache.find(path); it != m_PathCache.end())
			{
				return it->second;
			}
		}

		std::filesystem::path inputPath(path);
		std::filesystem::path resolvedPath;

		if (inputPath.is_absolute())
		{
			resolvedPath = inputPath;
		}
		else
		{
			std::string pathStr = inputPath.generic_string();
			bool isEngineResource = false;
			if (pathStr.find("engine/") == 0)
			{
				pathStr = pathStr.substr(7);
				isEngineResource = true;
			}

			if (isEngineResource)
			{
				if (!m_EngineRoot.empty())
				{
					std::filesystem::path candidate = m_EngineRoot / pathStr;
					if (std::filesystem::exists(candidate))
					{
						resolvedPath = candidate;
					}
				}
			}
			else
			{
				if (!m_AssetDirectory.empty())
				{
					std::filesystem::path candidate = m_AssetDirectory / pathStr;
					if (std::filesystem::exists(candidate))
					{
						resolvedPath = candidate;
					}
				}

				if (resolvedPath.empty() && !m_ProjectDirectory.empty())
				{
					std::filesystem::path candidate = m_ProjectDirectory / pathStr;
					if (std::filesystem::exists(candidate))
					{
						resolvedPath = candidate;
					}
				}
			}

			if (resolvedPath.empty())
			{
				if (!isEngineResource && !m_AssetDirectory.empty())
				{
					resolvedPath = m_AssetDirectory / pathStr;
				}
				else if (!m_EngineRoot.empty())
				{
					resolvedPath = m_EngineRoot / pathStr;
				}
				else
				{
					resolvedPath = m_ProjectDirectory / pathStr;
				}
			}
		}

		std::string resolved = std::filesystem::absolute(resolvedPath).lexically_normal().generic_string();

		std::lock_guard<std::mutex> lock(m_PathMutex);
		m_PathCache[path] = resolved;
		return resolved;
	}

	void AssetPathResolver::RegisterHandle(const std::string& path, AssetHandle handle)
	{
		std::string resolved = ResolvePath(path);
		std::lock_guard<std::mutex> lock(m_PathMutex);
		m_PathToHandle[resolved] = handle;
	}

	void AssetPathResolver::UnregisterHandle(AssetHandle handle)
	{
		std::lock_guard<std::mutex> lock(m_PathMutex);
		for (auto it = m_PathToHandle.begin(); it != m_PathToHandle.end();)
		{
			if (it->second == handle)
			{
				it = m_PathToHandle.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	AssetHandle AssetPathResolver::ResolveToHandle(const std::string& path) const
	{
		std::string resolved = ResolvePath(path);
		std::lock_guard<std::mutex> lock(m_PathMutex);
		if (auto it = m_PathToHandle.find(resolved); it != m_PathToHandle.end())
		{
			return it->second;
		}
		return AssetHandle(0);
	}

	void AssetPathResolver::ResetPath(const std::string& path)
	{
		std::lock_guard<std::mutex> lock(m_PathMutex);
		m_PathCache.erase(path);
	}

	void AssetPathResolver::ClearCache()
	{
		std::lock_guard<std::mutex> lock(m_PathMutex);
		m_PathCache.clear();
		m_PathToHandle.clear();
	}

} // namespace Chained
