#include "engine/core/platform_detection.h"
#include "engine/core/window.h"
#include "engine/platform/glfw/glfw_window.h"

namespace CHEngine
{

std::unique_ptr<Window> Window::Create(const WindowProperties& properties)
{
    // Currently, all supported platforms (Windows, Linux, MacOS) use GLFW
    return std::make_unique<GlfwWindow>(properties);
}

} // namespace CHEngine
