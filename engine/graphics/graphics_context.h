#ifndef CH_GRAPHICS_CONTEXT_H
#define CH_GRAPHICS_CONTEXT_H

namespace CHEngine
{
    // Interface for managing graphics API context (OpenGL, Vulkan, Raylib, etc.)
    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;
    };
}

#endif // CH_GRAPHICS_CONTEXT_H
