//========= Copyright Chained Decos, All rights reserved. ============//
//
// Purpose: Filesystem utilities for the scripting subsystem.
//          Provides path resolution helpers for managed assemblies.
//
//=============================================================================//
#include "script_file_system.h"

#ifdef CH_PLATFORM_WINDOWS
#include <windows.h>
#elif defined(CH_PLATFORM_LINUX)
#include <unistd.h>
#endif

namespace CHEngine {

//-----------------------------------------------------------------------------
// Purpose: Returns the directory containing the engine executable
//-----------------------------------------------------------------------------
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

} // namespace CHEngine
