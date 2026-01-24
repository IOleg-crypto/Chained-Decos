#include "process_utils.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <malloc.h>
#include <windows.h>

#endif

#ifdef _WIN32
#include <intrin.h>
#endif

#include <cstdint>

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

std::string GetCPUName()
{
#ifdef _WIN32
    int cpuInfo[4] = {-1};
    unsigned nExIds, i = 0;
    char cpuBrandString[0x40];
    memset(cpuBrandString, 0, sizeof(cpuBrandString));

    __cpuid(cpuInfo, 0x80000000);
    nExIds = cpuInfo[0];

    for (i = 0x80000002; i <= nExIds; ++i)
    {
        __cpuid(cpuInfo, i);
        if (i == 0x80000002)
            memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));
        else if (i == 0x80000003)
            memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));
        else if (i == 0x80000004)
            memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));
    }
    return std::string(cpuBrandString);
#else
    return "Unknown CPU";
#endif
}

std::string GetGPUName()
{
    // On Windows/Linux with OpenGL, this should ideally be glGetString(GL_RENDERER)
    // But ProcessUtils doesn't know about GL. We'll return an empty string
    // and let the Profiler fill it if it can.
    return "";
}

void GetSystemMemoryInfo(uint64_t &totalRAM, uint64_t &usedRAM)
{
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    totalRAM = memInfo.ullTotalPhys;
    usedRAM = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
#else
    totalRAM = 0;
    usedRAM = 0;
#endif
}

} // namespace ProcessUtils
} // namespace CHEngine
