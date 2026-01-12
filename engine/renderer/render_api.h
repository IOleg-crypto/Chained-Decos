#ifndef CH_RENDERER_API_H
#define CH_RENDERER_API_H

#include <memory>
#include <raylib.h>

namespace CHEngine
{
class RenderAPI
{
public:
    enum class API : std::uint8_t
    {
        None = 0,
        OpenGL = 1
    };

public:
    virtual ~RenderAPI() = default;

    virtual void Init() = 0;
    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual void SetClearColor(const Color &color) = 0;
    virtual void Clear() = 0;

    virtual void DrawIndexed(uint32_t indexCount) = 0;

    static API GetAPI()
    {
        return s_API;
    }
    static std::unique_ptr<RenderAPI> Create();

private:
    static API s_API;
};

} // namespace CHEngine

#endif // CH_RENDERER_API_H
