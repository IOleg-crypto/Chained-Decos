#ifndef CH_RAYLIB_CONTEXT_H
#define CH_RAYLIB_CONTEXT_H

#include "graphics_context.h"
#include <string>

struct GLFWwindow;

namespace CHEngine
{
    class RaylibContext : public GraphicsContext
    {
    public:
        RaylibContext(GLFWwindow* windowHandle);

        virtual void Init() override;
        virtual void SwapBuffers() override;

    private:
        GLFWwindow* m_WindowHandle;
    };
}

#endif // CH_RAYLIB_CONTEXT_H
