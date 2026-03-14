#include "engine/core/assert.h"
#include "engine/core/platform_detection.h"
#include "engine/core/window.h"

#ifdef CH_PLATFORM_WINDOWS
#include "engine/platform/windows/windows_window.h"
#elif defined(CH_PLATFORM_LINUX)
#include "engine/platform/linux/linux_window.h"
#endif

namespace CHEngine
{

std::unique_ptr<Window> Window::Create(const WindowProperties& properties)
{
#ifdef CH_PLATFORM_WINDOWS
    return std::make_unique<WindowsWindow>(properties);
#elif defined(CH_PLATFORM_LINUX)
    return std::make_unique<LinuxWindow>(properties);
#else
    CH_CORE_ASSERT(false, "Unknown platform!");
    return nullptr;
#endif
}

} // namespace CHEngine
