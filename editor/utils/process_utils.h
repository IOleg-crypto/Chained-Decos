#ifndef CD_EDITOR_UTILS_PROCESS_UTILS_H
#define CD_EDITOR_UTILS_PROCESS_UTILS_H

#include <string>

namespace CHEngine
{
namespace ProcessUtils
{
/**
 * @brief Launches an external process.
 * @param commandLine The full command line to execute.
 * @param workingDirectory The directory the process should start in.
 * @return true if the process was launched successfully, false otherwise.
 */
bool LaunchProcess(const std::string &commandLine, const std::string &workingDirectory = "");
} // namespace ProcessUtils
} // namespace CHEngine

#endif // CD_EDITOR_UTILS_PROCESS_UTILS_H
