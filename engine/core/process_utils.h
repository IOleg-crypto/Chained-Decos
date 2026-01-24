#ifndef CH_PROCESS_UTILS_H
#define CH_PROCESS_UTILS_H

#include <string>

#include <cstdint>
namespace CHEngine
{
namespace ProcessUtils
{
bool LaunchProcess(const std::string &commandLine);
std::string GetExecutablePath();
std::string GetCPUName();
std::string GetGPUName();
void GetSystemMemoryInfo(uint64_t &totalRAM, uint64_t &usedRAM);
} // namespace ProcessUtils
} // namespace CHEngine

#endif // CH_PROCESS_UTILS_H
