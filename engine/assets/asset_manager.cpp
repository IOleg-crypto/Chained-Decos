#include "engine/assets/asset_manager.h"
#include "engine/common/thread_pool.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"

#include "engine/assets/asset_metadata.h"
#include "engine/assets/loaders/font_loader.h"
#include "engine/assets/loaders/model_loader.h"
#include "engine/assets/loaders/audio_loader.h"
#include "engine/assets/loaders/material_loader.h"

#include "engine/assets/loaders/texture_loader.h"
#include "engine/assets/loaders/environment_loader.h"
#include "engine/assets/loaders/shader_loader.h"
#include "engine/assets/loaders/anim_graph_loader.h"

#include "engine/pack/dictionary_pack_reader.h"
#include "pack/reader.hpp"

namespace Chained
{
	constexpr size_t kMaxAssetFinalizationsPerFrame = 32;
	constexpr auto kMaxAssetFinalizeBudget = std::chrono::milliseconds(5);

	AssetManager::AssetManager() = default;

	void AssetManager::Initialize()
	{
		RegisterLoader(AssetType::Model, std::make_unique<ModelLoader>());
		RegisterLoader(AssetType::Texture, std::make_unique<TextureLoader>());
		RegisterLoader(AssetType::Shader, std::make_unique<ShaderLoader>());
		RegisterLoader(AssetType::Environment, std::make_unique<EnvironmentLoader>());
		RegisterLoader(AssetType::Font, std::make_unique<FontLoader>());
		RegisterLoader(AssetType::Audio, std::make_unique<AudioLoader>());
		RegisterLoader(AssetType::Material, std::make_unique<MaterialLoader>());
		RegisterLoader(AssetType::AnimationGraph, std::make_unique<AnimGraphLoader>());
	}

	void AssetManager::Shutdown()
	{
		constexpr int kMaxIter = 5000;
		int iter = 0;
		while (HasBackgroundWork() && iter++ < kMaxIter)
		{
			FinalizePendingLoads();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		if (iter >= kMaxIter)
		{
			CH_CORE_ERROR("AssetManager: Shutdown timed out — some assets may still be loading.");
		}
		if (auto* tp = ServiceLocator::TryGet<ThreadPool>())
		{
			tp->WaitIdle();
		}
	}

	void AssetManager::Update(Timestep ts)
	{
		if (m_HotReloadInterval > 0.0f)
		{
			m_HotReloadAccumulator += ts.GetSeconds();
			if (m_HotReloadAccumulator >= m_HotReloadInterval)
			{
				m_HotReloadAccumulator = 0.0f;
				CheckAssetHotReload();
			}
		}

		FinalizePendingLoads();
	}

	std::vector<uint8_t> AssetManager::TryPackFallback(const std::string& packKey) const
	{
		if (!m_PackOpen || (m_PackReaders.empty() && m_DictPackReaders.empty()))
		{
			return {};
		}

		if (packKey.rfind("assets/", 0) != 0 && packKey.rfind("resources/", 0) != 0)
		{
			std::string altAssets = "assets/" + packKey;
			std::string altResources = "resources/" + packKey;

			// Check standard pack readers
			for (auto it = m_PackReaders.rbegin(); it != m_PackReaders.rend(); ++it)
			{
				auto& reader = *it;
				uint64_t idx = 0;
				if (reader->getItemIndex(altAssets.c_str(), idx))
				{
					std::vector<uint8_t> data;
					reader->readItemData(idx, data);
					return data;
				}
				if (reader->getItemIndex(altResources.c_str(), idx))
				{
					std::vector<uint8_t> data;
					reader->readItemData(idx, data);
					return data;
				}
			}

			// Check dictionary pack readers
			for (auto it = m_DictPackReaders.rbegin(); it != m_DictPackReaders.rend(); ++it)
			{
				auto& reader = *it;
				uint64_t idx = 0;
				if (reader->GetItemIndex(altAssets.c_str(), idx))
				{
					std::vector<uint8_t> data;
					reader->ReadItemData(idx, data);
					return data;
				}
				if (reader->GetItemIndex(altResources.c_str(), idx))
				{
					std::vector<uint8_t> data;
					reader->ReadItemData(idx, data);
					return data;
				}
			}
		}
		return {};
	}

	bool AssetManager::TryPackFallbackExists(const std::string& packKey) const
	{
		if (!m_PackOpen || (m_PackReaders.empty() && m_DictPackReaders.empty()))
		{
			return false;
		}

		if (packKey.rfind("assets/", 0) != 0 && packKey.rfind("resources/", 0) != 0)
		{
			std::string altAssets = "assets/" + packKey;
			std::string altResources = "resources/" + packKey;

			// Check standard pack readers
			for (auto it = m_PackReaders.rbegin(); it != m_PackReaders.rend(); ++it)
			{
				auto& reader = *it;
				uint64_t idx = 0;
				if (reader->getItemIndex(altAssets.c_str(), idx) || reader->getItemIndex(altResources.c_str(), idx))
				{
					return true;
				}
			}

			// Check dictionary pack readers
			for (auto it = m_DictPackReaders.rbegin(); it != m_DictPackReaders.rend(); ++it)
			{
				auto& reader = *it;
				uint64_t idx = 0;
				if (reader->GetItemIndex(altAssets.c_str(), idx) || reader->GetItemIndex(altResources.c_str(), idx))
				{
					return true;
				}
			}
		}
		return false;
	}

	std::vector<AssetManager::StaleAsset> AssetManager::CollectStaleAssets(int thresholdSeconds) const
	{
		CH_PROFILE_FUNCTION();
		std::vector<StaleAsset> stale;

		// Snapshot the cache under a short lock
		std::vector<std::pair<AssetHandle, std::shared_ptr<Asset>>> snapshot;
		{
			std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
			snapshot.reserve(m_AssetCache.size());
			for (const auto& [handle, asset] : m_AssetCache)
			{
				if (asset && asset->GetState() != AssetState::Loading)
				{
					snapshot.emplace_back(handle, asset);
				}
			}
		}

		auto now = std::filesystem::file_time_type::clock::now();
		for (const auto& [handle, asset] : snapshot)
		{
			AssetType type = asset->GetType();
			if (type != AssetType::Model && type != AssetType::Texture && type != AssetType::Material)
			{
				continue;
			}

			const std::string& path = asset->GetPath();
			if (path.empty())
			{
				continue;
			}

			std::error_code ec;
			auto fileTime = std::filesystem::last_write_time(path, ec);
			if (ec)
			{
				continue;
			}

			auto age = std::chrono::duration_cast<std::chrono::seconds>(now - fileTime).count();
			if (age < thresholdSeconds)
			{
				stale.push_back({handle, type, path});
			}
		}

		return stale;
	}

	void AssetManager::CheckAssetHotReload()
	{
		auto stale = CollectStaleAssets(10);
		for (const auto& [handle, type, path] : stale)
		{
			CH_CORE_INFO("AssetManager: Hot-reloading recently modified {} '{}'",
						 type == AssetType::Model	  ? "model"
						 : type == AssetType::Texture ? "texture"
													  : "material",
						 std::filesystem::path(path).filename().string());
			ReloadAsset(handle, type);
		}
	}

	AssetManager::~AssetManager()
	{
		std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
		m_PackOpen = false;
		m_AssetCache.clear();
		m_PathResolver.ClearCache();
		m_Loaders.clear();
	}

	void AssetManager::RegisterLoader(AssetType type, std::unique_ptr<IAssetLoader> loader)
	{
		m_Loaders[type] = std::move(loader);
	}

	bool AssetManager::ExecuteLoad(const std::shared_ptr<Asset>& asset, IAssetLoader* loader,
								   const std::string& resolved)
	{
		try
		{
			std::string loaderError;
			if (!loader->Load(asset, resolved, &loaderError))
			{
				asset->Fail(loaderError.empty() ? ("AssetManager: Load failed for '" + resolved + "'") : loaderError);
				return false;
			}

			asset->ClearError();
			return true;
		} catch (const std::exception& e)
		{
			asset->Fail(std::string("AssetManager: Load exception for '") + resolved + "': " + e.what());
		} catch (...)
		{
			asset->Fail(std::string("AssetManager: Load exception for '") + resolved + "' with unknown exception");
		}
		return false;
	}

	std::shared_ptr<Asset> AssetManager::LoadAsset(const std::string& path, AssetType type)
	{
		if (path.empty() || path.front() == '*')
		{
			return nullptr;
		}

		std::string resolved = ResolvePath(path);

		IAssetLoader* loader = nullptr;
		std::shared_ptr<Asset> asset;

		{
			std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
			if (auto it = m_PathResolver.ResolveToHandle(resolved); it != AssetHandle(0))
			{
				auto handle = it;
				if (auto currentIt = m_AssetCache.find(handle); currentIt != m_AssetCache.end())
				{
					return currentIt->second;
				}
			}

			auto loaderIt = m_Loaders.find(type);
			if (loaderIt == m_Loaders.end())
			{
				CH_CORE_ERROR("AssetManager: No loader registered for type {}", (int)type);
				return nullptr;
			}

			asset = loaderIt->second->Create();
			if (!asset)
			{
				return nullptr;
			}

			loader = loaderIt->second.get();

			// Load or create .meta sidecar to get a stable UUID
			auto meta = MetaUtils::LoadOrCreateMeta(resolved, type);
			AssetHandle stableHandle = meta.uuid;
			asset->SetID(stableHandle);
			asset->SetPath(resolved);
			asset->SetState(AssetState::Loading);
			asset->ClearError();
			m_AssetCache[stableHandle] = asset;
			m_PathResolver.RegisterHandle(resolved, stableHandle);
		}

		if (!loader->IsAsync())
		{
			if (ExecuteLoad(asset, loader, resolved))
			{
				asset->OnLoaded();
				asset->SetState(AssetState::Ready);
			}
		}
		else
		{
			if (auto* tp = ServiceLocator::TryGet<ThreadPool>())
			{
				tp->QueueTask([this, asset, loader, resolved]() {
					if (ExecuteLoad(asset, loader, resolved))
					{
						std::lock_guard<std::mutex> lock(m_PendingMutex);
						m_PendingAssets.push_back(asset);
					}
				});
			}
		}

		return asset;
	}

	std::shared_ptr<Asset> AssetManager::GetAsset(AssetHandle handle)
	{
		if (handle != 0)
		{
			std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
			if (auto it = m_AssetCache.find(handle); it != m_AssetCache.end())
			{
				return it->second;
			}
		}

		return nullptr;
	}

	void AssetManager::FinalizePendingLoads()
	{
		CH_PROFILE_FUNCTION();

		const auto updateStart = std::chrono::steady_clock::now();
		size_t finalizedCount = 0;

		while (finalizedCount < kMaxAssetFinalizationsPerFrame)
		{
			std::shared_ptr<Asset> asset;
			{
				std::lock_guard<std::mutex> lock(m_PendingMutex);
				if (m_PendingAssets.empty())
				{
					return;
				}

				asset = std::move(m_PendingAssets.front());
				m_PendingAssets.pop_front();
			}

			if (!asset)
			{
				continue;
			}

			try
			{
				asset->ClearError();
				asset->OnLoaded();
				if (asset->GetState() != AssetState::Failed)
				{
					asset->SetState(AssetState::Ready);
				}
			} catch (const std::exception& e)
			{
				asset->Fail(std::string("AssetManager: Finalization failed for '") + asset->GetPath() +
							"': " + e.what());
			} catch (...)
			{
				asset->Fail(std::string("AssetManager: Finalization failed for '") + asset->GetPath() +
							"' with an unknown exception");
			}

			++finalizedCount;

			if ((std::chrono::steady_clock::now() - updateStart) >= kMaxAssetFinalizeBudget)
			{
				break;
			}
		}
	}

	size_t AssetManager::GetPendingFinalizeCount() const
	{
		std::lock_guard<std::mutex> lock(m_PendingMutex);
		return m_PendingAssets.size();
	}

	size_t AssetManager::GetLoadingAssetCount() const
	{
		std::lock_guard<std::recursive_mutex> lock(m_AssetLock);

		size_t loadingCount = 0;
		for (const auto& [handle, asset] : m_AssetCache)
		{
			(void)handle;
			if (asset && asset->GetState() == AssetState::Loading)
			{
				++loadingCount;
			}
		}

		return loadingCount;
	}

	bool AssetManager::HasBackgroundWork() const
	{
		{
			std::lock_guard<std::mutex> lock(m_PendingMutex);
			if (!m_PendingAssets.empty())
			{
				return true;
			}
		}
		std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
		for (const auto& [handle, asset] : m_AssetCache)
		{
			if (asset && asset->GetState() == AssetState::Loading)
			{
				return true;
			}
		}
		return false;
	}
	void AssetManager::ReloadAsset(AssetHandle handle, AssetType type)
	{
		std::shared_ptr<Asset> asset;
		IAssetLoader* loader = nullptr;
		std::string path;

		{
			std::lock_guard<std::recursive_mutex> lock(m_AssetLock);

			auto it = m_AssetCache.find(handle);
			if (it == m_AssetCache.end())
			{
				return;
			}

			auto loaderIt = m_Loaders.find(type);
			if (loaderIt == m_Loaders.end())
			{
				return;
			}

			asset = it->second;
			loader = loaderIt->second.get();
			path = asset->GetPath();
			if (path.empty())
			{
				return;
			}
		}

		std::string resolved = ResolvePath(path);
		asset->SetState(AssetState::Loading);
		if (ExecuteLoad(asset, loader, resolved))
		{
			asset->OnLoaded();
			asset->SetState(AssetState::Ready);

			// Update content hash in .meta after successful reload
			if (std::filesystem::exists(resolved))
			{
				auto metaPath = MetaUtils::GetMetaPath(resolved);
				if (std::filesystem::exists(metaPath))
				{
					auto meta = MetaUtils::ReadMeta(metaPath);
					meta.contentHash = MetaUtils::ComputeFileHash(resolved);
					MetaUtils::WriteMeta(metaPath, meta);
				}
			}
		}
	}

	void AssetManager::Invalidate(const std::string& path, bool deleteFromDisk)
	{
		if (path.empty())
		{
			return;
		}

		std::string resolved = ResolvePath(path);

		if (deleteFromDisk)
		{
			DeleteChasset(resolved);
		}

		std::lock_guard<std::recursive_mutex> lock(m_AssetLock);

		AssetHandle handle = m_PathResolver.ResolveToHandle(resolved);
		if (handle != AssetHandle(0))
		{
			m_AssetCache.erase(handle);
		}
		m_PathResolver.ResetPath(path);

		CH_CORE_INFO("AssetManager: Invalidated cache for '{}'", resolved);
	}

	void AssetManager::Unload(AssetHandle handle)
	{
		if (handle == AssetHandle(0))
		{
			return;
		}

		std::shared_ptr<Asset> asset;
		{
			std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
			if (auto it = m_AssetCache.find(handle); it != m_AssetCache.end())
			{
				asset = it->second;
			}
		}

		asset->Unload();

		{
			std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
			m_AssetCache.erase(handle);
		}

		// Remove path-to-handle mappings for this handle
		m_PathResolver.UnregisterHandle(handle);

		CH_CORE_INFO("AssetManager: Unloaded asset {}", (uint64_t)handle);
	}

	void AssetManager::Unload(const std::string& path)
	{
		AssetHandle handle = m_PathResolver.ResolveToHandle(path);
		if (handle != AssetHandle(0))
		{
			Unload(handle);
		}
	}

	void AssetManager::UnloadUnused()
	{
		std::vector<AssetHandle> toUnload;
		{
			std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
			for (const auto& [handle, asset] : m_AssetCache)
			{
				if (asset && asset.use_count() == 1)
				{
					toUnload.push_back(handle);
				}
			}
		}

		for (AssetHandle handle : toUnload)
		{
			Unload(handle);
		}

		if (!toUnload.empty())
		{
			CH_CORE_INFO("AssetManager: Unloaded {} unused assets", toUnload.size());
		}
	}

	bool AssetManager::DeleteChasset(const std::string& path)
	{
		std::filesystem::path modelPath = path;
		std::filesystem::path chassetPath = modelPath;
		chassetPath.replace_extension(".chasset");

		std::error_code ec;
		if (std::filesystem::exists(chassetPath, ec))
		{
			std::filesystem::remove(chassetPath, ec);
			if (!ec)
			{
				CH_CORE_INFO("AssetManager: Deleted .chasset '{}'", chassetPath.filename().string());
				return true;
			}
			CH_CORE_WARN("AssetManager: Failed to delete .chasset '{}': {}", chassetPath.string(), ec.message());
		}
		return false;
	}

	size_t AssetManager::DeleteAllChassets()
	{
		if (m_PathResolver.GetAssetDirectory().empty())
		{
			CH_CORE_WARN("AssetManager: Asset directory not set, cannot clear .chasset cache");
			return 0;
		}

		size_t deleted = 0;
		std::error_code ec;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(m_PathResolver.GetAssetDirectory(), ec))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".chasset")
			{
				std::filesystem::remove(entry.path(), ec);
				if (!ec)
				{
					CH_CORE_INFO("AssetManager: Deleted .chasset '{}'", entry.path().filename().string());
					++deleted;
				}
				else
				{
					CH_CORE_WARN("AssetManager: Failed to delete .chasset '{}': {}", entry.path().string(),
								 ec.message());
				}
			}
		}

		CH_CORE_INFO("AssetManager: Cleared .chasset cache ({} file(s) deleted)", deleted);

		if (deleted > 0)
		{
			std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
			m_AssetCache.clear();
			m_PathResolver.ClearCache();
		}

		return deleted;
	}

	std::vector<std::string> AssetManager::GetStaleAssets() const
	{
		auto stale = CollectStaleAssets(30);
		std::vector<std::string> paths;
		paths.reserve(stale.size());
		for (auto& s : stale)
		{
			paths.push_back(std::move(s.path));
		}
		return paths;
	}

	size_t AssetManager::ReloadAllStale()
	{
		auto stale = CollectStaleAssets(30);
		for (const auto& [handle, type, path] : stale)
		{
			ReloadAsset(handle, type);
		}

		if (!stale.empty())
		{
			CH_CORE_INFO("AssetManager: Reloaded {} stale assets", stale.size());
		}

		return stale.size();
	}

	bool AssetManager::OpenPack(const std::filesystem::path& packPath)
	{
		std::lock_guard<std::recursive_mutex> lock(m_AssetLock);

		for (const auto& p : m_OpenedPackPaths)
		{
			if (p == packPath)
			{
				return true;
			}
		}

		if (!std::filesystem::exists(packPath))
		{
			CH_CORE_WARN("AssetManager: Pack file not found: {}", packPath.string());
			return false;
		}

		// Read header to detect dictionary packs
		FILE* headerFile = fopen(packPath.string().c_str(), "rb");
		if (!headerFile)
		{
			CH_CORE_ERROR("AssetManager: Failed to open pack '{}' for header check", packPath.string());
			return false;
		}

		PackHeader packHeader;
		bool hasDict = false;
		if (fread(&packHeader, sizeof(PackHeader), 1, headerFile) == 1)
		{
			hasDict = packHeader._reserved != 0;
		}
		fclose(headerFile);

		try
		{
			if (hasDict)
			{
				auto dictReader = std::make_unique<DictionaryPackReader>();
				if (dictReader->Open(packPath))
				{
					CH_CORE_INFO("AssetManager: Opened dictionary pack '{}' ({} items)", packPath.string(),
								 dictReader->GetItemCount());
					m_OpenedPackPaths.push_back(packPath);
					m_DictPackReaders.push_back(std::move(dictReader));
					m_PackOpen = true;
					return true;
				}
				CH_CORE_WARN("AssetManager: Failed to open as dictionary pack, trying standard pack");
			}

			auto reader = std::make_unique<pack::Reader>(packPath);
			CH_CORE_INFO("AssetManager: Opened pack '{}' ({} items)", packPath.string(), reader->getItemCount());
			m_OpenedPackPaths.push_back(packPath);
			m_PackReaders.push_back(std::move(reader));
			m_PackOpen = true;
			return true;
		} catch (const pack::Error& err)
		{
			CH_CORE_ERROR("AssetManager: Failed to open pack '{}': {}", packPath.string(), err.what());
			return false;
		}
	}

	size_t AssetManager::OpenAllPacksInDirectory(const std::filesystem::path& dir)
	{
		std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
		std::error_code ec;
		if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec))
		{
			return 0;
		}

		std::vector<std::filesystem::path> packFiles;
		for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".pack")
			{
				packFiles.push_back(entry.path());
			}
		}

		std::sort(packFiles.begin(), packFiles.end());

		size_t openedCount = 0;
		for (const auto& p : packFiles)
		{
			if (OpenPack(p))
			{
				++openedCount;
			}
		}

		return openedCount;
	}

	void AssetManager::CloseAllPacks()
	{
		std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
		m_PackReaders.clear();
		m_DictPackReaders.clear();
		m_OpenedPackPaths.clear();
		m_PackOpen = false;
	}

	std::vector<uint8_t> AssetManager::ReadAssetData(const std::string& assetPath)
	{
		{
			std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
			if (m_PackOpen && (!m_PackReaders.empty() || !m_DictPackReaders.empty()))
			{
				std::string packKey = m_PathResolver.ResolvePackKey(assetPath);

				// Check standard pack readers
				for (auto it = m_PackReaders.rbegin(); it != m_PackReaders.rend(); ++it)
				{
					auto& reader = *it;
					uint64_t idx = 0;
					if (reader->getItemIndex(packKey.c_str(), idx))
					{
						std::vector<uint8_t> data;
						reader->readItemData(idx, data);
						return data;
					}
				}

				// Check dictionary pack readers
				for (auto it = m_DictPackReaders.rbegin(); it != m_DictPackReaders.rend(); ++it)
				{
					auto& reader = *it;
					uint64_t idx = 0;
					if (reader->GetItemIndex(packKey.c_str(), idx))
					{
						std::vector<uint8_t> data;
						reader->ReadItemData(idx, data);
						return data;
					}
				}

				auto data = TryPackFallback(packKey);
				if (!data.empty())
				{
					return data;
				}
			}
		}

		std::ifstream file(assetPath, std::ios::binary | std::ios::ate);
		if (file.is_open())
		{
			auto size = file.tellg();
			file.seekg(0);
			std::vector<uint8_t> data(static_cast<size_t>(size));
			file.read(reinterpret_cast<char*>(data.data()), size);
			return data;
		}

		return {};
	}

	std::string AssetManager::ReadText(const std::string& path)
	{
		auto data = ReadAssetData(path);
		if (data.empty())
		{
			return {};
		}
		return std::string(data.begin(), data.end());
	}

	bool AssetManager::FileExists(const std::string& path) const
	{
		std::lock_guard<std::recursive_mutex> lock(m_AssetLock);
		if (m_PackOpen && (!m_PackReaders.empty() || !m_DictPackReaders.empty()))
		{
			std::string packKey = m_PathResolver.ResolvePackKey(path);

			// Check standard pack readers
			for (auto it = m_PackReaders.rbegin(); it != m_PackReaders.rend(); ++it)
			{
				auto& reader = *it;
				uint64_t idx = 0;
				if (reader->getItemIndex(packKey.c_str(), idx))
				{
					return true;
				}
			}

			// Check dictionary pack readers
			for (auto it = m_DictPackReaders.rbegin(); it != m_DictPackReaders.rend(); ++it)
			{
				auto& reader = *it;
				uint64_t idx = 0;
				if (reader->GetItemIndex(packKey.c_str(), idx))
				{
					return true;
				}
			}

			if (TryPackFallbackExists(packKey))
			{
				return true;
			}
		}

		std::error_code ec;
		std::string resolved = ResolvePath(path);
		if (!resolved.empty() && std::filesystem::exists(resolved, ec))
		{
			return true;
		}
		return std::filesystem::exists(path, ec);
	}

	bool AssetManager::HasAsset(const std::string& path) const
	{
		return FileExists(path);
	}

	void AssetManager::EnumeratePackedPaths(const std::function<void(std::string_view)>& callback) const
	{
		if (!m_PackOpen || (m_PackReaders.empty() && m_DictPackReaders.empty()))
		{
			return;
		}
		std::unordered_set<std::string_view> seen;

		// Enumerate standard pack readers
		for (const auto& reader : m_PackReaders)
		{
			const uint64_t count = reader->getItemCount();
			for (uint64_t i = 0; i < count; ++i)
			{
				std::string_view itemPath = reader->getItemPath(i);
				if (seen.insert(itemPath).second)
				{
					callback(itemPath);
				}
			}
		}

		// Enumerate dictionary pack readers
		for (const auto& reader : m_DictPackReaders)
		{
			const uint64_t count = reader->GetItemCount();
			for (uint64_t i = 0; i < count; ++i)
			{
				std::string itemPath = reader->GetItemPath(i);
				// Skip dictionary item
				if (itemPath == "__zstd_dictionary__")
				{
					continue;
				}
				if (seen.insert(itemPath).second)
				{
					callback(itemPath);
				}
			}
		}
	}

	std::vector<uint8_t> AssetManager::ReadProjectAsset(const std::filesystem::path& absolutePath)
	{
		if (!m_PackOpen || (m_PackReaders.empty() && m_DictPackReaders.empty()) ||
			m_PathResolver.GetProjectDirectory().empty())
		{
			return {};
		}
		std::error_code ec;
		auto rel = std::filesystem::relative(absolutePath, m_PathResolver.GetProjectDirectory(), ec);
		if (ec || rel.empty())
		{
			return {};
		}
		return ReadAssetData(rel.generic_string());
	}

} // namespace Chained
