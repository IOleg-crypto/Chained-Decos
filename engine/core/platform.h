#ifndef CH_PLATFORM_H
#define CH_PLATFORM_H

#include "engine/common/base.h"
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace Chained
{
class CH_API Platform
{
public:
    // Returns the absolute path to the directory containing the current executable.
    static std::filesystem::path GetExecutableDirectory();
    // Returns the current system time in seconds.
    static float GetTime();
    // Sleeps the current thread for the specified milliseconds.
    static void Sleep(uint32_t milliseconds);
};
} // namespace Chained

#endif // CH_PLATFORM_H
