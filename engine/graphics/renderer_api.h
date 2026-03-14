#ifndef CH_RENDERER_API_H
#define CH_RENDERER_API_H

#include "raylib.h"
#include <memory>

namespace CHEngine
{
class VertexArray;

class RendererAPI
{
public:
    enum class API
    {
        None = 0,
        Raylib = 1
    };

public:
    virtual ~RendererAPI() = default;

    virtual void Init() = 0;
    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void SetClearColor(const Color& color) = 0;
    virtual void Clear() = 0;

    virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

    static API GetAPI() { return s_API; }
    static std::unique_ptr<RendererAPI> Create();

private:
    static API s_API;
};

} // namespace CHEngine

#endif // CH_RENDERER_API_H
