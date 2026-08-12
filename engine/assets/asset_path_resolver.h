#ifndef CH_ASSET_PATH_RESOLVER_H
#define CH_ASSET_PATH_RESOLVER_H

#include "engine/assets/asset.h"
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Chained
{

	class AssetPathResolver
	{
	public:
		void SetAssetDirectory(const std::filesystem::path& path)
		{
			m_AssetDirectory = path;
		}
		void SetProjectDirectory(const std::filesystem::path& path)
		{
			m_ProjectDirectory = path;
		}
		void SetEngineRoot(const std::filesystem::path& path)
		{
			m_EngineRoot = path;
		}

	public:
		std::string ResolvePath(const std::string& path) const;
		AssetHandle ResolveToHandle(const std::string& path) const;
		void RegisterHandle(const std::string& path, AssetHandle handle);
		void UnregisterHandle(AssetHandle handle);

		void ResetPath(const std::string& path);
		void ClearCache();

		std::string ResolvePackKey(const std::string& assetPath) const;

	public:
		const std::filesystem::path& GetAssetDirectory() const
		{
			return m_AssetDirectory;
		}
		const std::filesystem::path& GetProjectDirectory() const
		{
			return m_ProjectDirectory;
		}
		const std::filesystem::path& GetEngineRoot() const
		{
			return m_EngineRoot;
		}

	private:
		mutable std::mutex m_PathMutex;
		mutable std::unordered_map<std::string, std::string> m_PathCache;
		mutable std::unordered_map<std::string, AssetHandle> m_PathToHandle;

		std::filesystem::path m_AssetDirectory;
		std::filesystem::path m_ProjectDirectory;
		std::filesystem::path m_EngineRoot;
	};

} // namespace Chained

#endif // CH_ASSET_PATH_RESOLVER_H
