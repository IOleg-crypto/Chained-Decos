#ifndef RENDERERAPI_H
#define RENDERERAPI_H

#include <memory>
#include <raylib.h>

namespace CHEngine
{

class RendererAPI
{
public:
    enum class API
    {
        None = 0,
        OpenGL = 1
    };

public:
    virtual ~RendererAPI() = default;

    virtual void Init() = 0;
    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual void SetClearColor(const Vector4 &color) = 0;
    virtual void Clear() = 0;

    virtual void DrawIndexed(uint32_t indexCount) = 0;

    static API GetAPI()
    {
        return s_API;
    }
    static std::unique_ptr<RendererAPI> Create();

private:
    static API s_API;
};

} // namespace CHEngine

#endif // RENDERERAPI_H
