#include "process_utils.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <malloc.h>
#include <windows.h>

#endif

namespace CHEngine
{
namespace ProcessUtils
{
bool LaunchProcess(const std::string &commandLine)
{
#ifdef _WIN32
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char *cmd = _strdup(commandLine.c_str());
    bool success = CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    if (success)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    free(cmd);
    return success;
#else
    // Fallback for other platforms
    return std::system((commandLine + " &").c_str()) == 0;
#endif
}

std::string GetExecutablePath()
{
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::string(path);
#else
    // Fallback for other platforms
    return "";
#endif
}
} // namespace ProcessUtils
} // namespace CHEngine
