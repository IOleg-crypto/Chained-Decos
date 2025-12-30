#include "ProcessUtils.h"
#include "core/Log.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NOGDI
#define NOUSER
#include <windows.h>
#else
#include <cstdlib>
#endif

namespace CHEngine
{
namespace ProcessUtils
{
bool LaunchProcess(const std::string &commandLine, const std::string &workingDirectory)
{
#ifdef _WIN32
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};

    const char *workingDir = workingDirectory.empty() ? nullptr : workingDirectory.c_str();

    if (CreateProcessA(nullptr,                    // Application name
                       (LPSTR)commandLine.c_str(), // Command line
                       nullptr,                    // Process security attributes
                       nullptr,                    // Thread security attributes
                       FALSE,                      // Inherit handles
                       0,                          // Creation flags
                       nullptr,                    // Environment
                       workingDir,                 // Working directory
                       &si,                        // Startup info
                       &pi))                       // Process information
    {
        CD_CORE_INFO("[ProcessUtils] Process launched successfully (PID: %lu)", pi.dwProcessId);

        // Close handles (we don't need to track the process)
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    else
    {
        CD_CORE_ERROR("[ProcessUtils] Failed to launch process (Error: %lu). Command: %s",
                      GetLastError(), commandLine.c_str());
        return false;
    }
#else
    // Unix/Linux: use system() or fork/exec
    std::string command = commandLine + " &";
    int result = std::system(command.c_str());
    if (result == 0)
    {
        CD_CORE_INFO("[ProcessUtils] Process launched successfully");
        return true;
    }
    else
    {
        CD_CORE_ERROR("[ProcessUtils] Failed to launch process (Exit code: %d). Command: %s",
                      result, commandLine.c_str());
        return false;
    }
#endif
}
} // namespace ProcessUtils
} // namespace CHEngine
