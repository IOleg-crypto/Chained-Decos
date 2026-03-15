#include "script_file_system.h"
#include "engine/core/log.h"
#include <chrono>
#include <system_error>

#ifdef CH_PLATFORM_WINDOWS
#include <windows.h>
#elif defined(CH_PLATFORM_LINUX)
#include <unistd.h>
#endif

namespace CHEngine {

	std::filesystem::path ScriptFileSystem::GetExecutableDir()
	{
#ifdef CH_PLATFORM_WINDOWS
		wchar_t path[MAX_PATH];
		GetModuleFileNameW(NULL, path, MAX_PATH);
		return std::filesystem::path(path).parent_path();
#elif defined(CH_PLATFORM_LINUX)
		char path[1024];
		ssize_t count = readlink("/proc/self/exe", path, sizeof(path));
		if (count != -1)
			return std::filesystem::path(std::string(path, count)).parent_path();
#endif
		return std::filesystem::current_path();
	}

	std::filesystem::path ScriptFileSystem::GetShadowDir()
	{
		return std::filesystem::temp_directory_path() / "CHEngine_shadow";
	}

	std::filesystem::path ScriptFileSystem::ShadowCopyDll(const std::filesystem::path& original)
	{
		auto shadowDir = GetShadowDir();
		std::filesystem::create_directories(shadowDir);

		auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
		std::string shadowName = original.stem().string() + "_" + std::to_string(ts) + ".dll";
		std::filesystem::path shadowDll = shadowDir / shadowName;

		std::error_code ec;
		std::filesystem::copy_file(original, shadowDll,
								   std::filesystem::copy_options::overwrite_existing, ec);
		if (ec)
		{
			CH_CORE_ERROR("ScriptFileSystem: Failed to shadow-copy '{}' -> '{}': {}",
						  original.string(), shadowDll.string(), ec.message());
			return original; // fall back to loading original
		}

		// Copy .pdb for debugger support
		auto pdbOrig = original; 
        pdbOrig.replace_extension(".pdb");
		if (std::filesystem::exists(pdbOrig))
		{
			auto pdbShadow = shadowDll; pdbShadow.replace_extension(".pdb");
			std::filesystem::copy_file(pdbOrig, pdbShadow,
									   std::filesystem::copy_options::overwrite_existing, ec);
		}

		CH_CORE_INFO("ScriptFileSystem: Shadow-copied '{}' -> '{}'", original.string(), shadowDll.string());
		return shadowDll;
	}

	void ScriptFileSystem::CleanupShadowCopies()
	{
		auto shadowDir = GetShadowDir();
		std::error_code ec;
		if (std::filesystem::exists(shadowDir, ec))
		{
			std::filesystem::remove_all(shadowDir, ec);
			if (ec)
				CH_CORE_WARN("ScriptFileSystem: Could not clean shadow dir: {}", ec.message());
		}
	}

} // namespace CHEngine
