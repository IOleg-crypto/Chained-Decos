#include "raylib_context.h"
#include "engine/core/log.h"
#include "engine/core/assert.h"
#include "raylib.h"

// GLFW for buffer swapping if needed, though Raylib usually handles it via EndDrawing
#ifndef GLFW_INCLUDE_NONE
    #define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace CHEngine
{
    RaylibContext::RaylibContext(GLFWwindow* windowHandle)
        : m_WindowHandle(windowHandle)
    {
        CH_CORE_ASSERT(windowHandle, "Window handle is null!");
    }

    void RaylibContext::Init()
    {
        CH_CORE_INFO("Raylib Graphics Context Initialized.");
        // Raylib handles its own GL state initialization via InitWindow which is called by the Window class
    }

    void RaylibContext::SwapBuffers()
    {
        // Raylib's EndDrawing() internally calls glfwSwapBuffers.
        // If we move to a more custom GL setup later, we'd do it here.
    }
}
