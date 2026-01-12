#ifndef CH_RENDER_COMMAND_H
#define CH_RENDER_COMMAND_H

#include <raylib.h>

namespace CHEngine
{
class RenderCommand
{
public:
    static void Init();

    static void SetClearColor(const Color &color);
    static void Clear();

    static void SetViewport(int x, int y, int width, int height);

    static void DrawIndexed(unsigned int count);

    static void SetDepthTest(bool enabled);
    static void SetDepthWrite(bool enabled);
    static void SetBlending(bool enabled);
};
} // namespace CHEngine

#endif // CH_RENDER_COMMAND_H
