#ifndef CH_SCRIPT_FILE_SYSTEM_H
#define CH_SCRIPT_FILE_SYSTEM_H

#include <filesystem>
#include <string>

namespace CHEngine {

	class ScriptFileSystem
	{
	public:
		// Returns the directory containing the engine executable
		static std::filesystem::path GetExecutableDir();

		// Returns the path to the temporary directory used for shadow copying assemblies
		static std::filesystem::path GetShadowDir();

		// Copies a DLL (and its .pdb if present) to a uniquely-named file in the shadow directory.
		// Returns the path to the newly created shadow copy.
		static std::filesystem::path ShadowCopyDll(const std::filesystem::path& original);

		// Deletes all old shadow copies in the shadow directory.
		static void CleanupShadowCopies();
	};

} // namespace CHEngine
#endif // CH_SCRIPT_FILE_SYSTEM_H
