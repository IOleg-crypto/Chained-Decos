#ifndef CH_PROCESS_UTILS_H
#define CH_PROCESS_UTILS_H

#include <string>

namespace CHEngine
{
namespace ProcessUtils
{
bool LaunchProcess(const std::string &commandLine);
std::string GetExecutablePath();
} // namespace ProcessUtils
} // namespace CHEngine

#endif // CH_PROCESS_UTILS_H
