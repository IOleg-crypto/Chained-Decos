//========= Copyright Chained Decos, All rights reserved. ============//
//
// Purpose: Filesystem utilities for the scripting subsystem.
//
//=============================================================================//
#ifndef CH_SCRIPT_FILE_SYSTEM_H
#define CH_SCRIPT_FILE_SYSTEM_H

#include <filesystem>
#include <string>

namespace CHEngine {

class ScriptFileSystem
{
public:
    //-----------------------------------------------------------------------------
    // Purpose: Returns the directory containing the engine executable
    //-----------------------------------------------------------------------------
    static std::filesystem::path GetExecutableDir();
};

} // namespace CHEngine
#endif // CH_SCRIPT_FILE_SYSTEM_H
