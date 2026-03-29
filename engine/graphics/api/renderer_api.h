#ifndef CH_RENDERER_API_H
#define CH_RENDERER_API_H

#include "engine/core/ch_math.h"
#include <memory>

namespace CHEngine
{
class VertexArray;

class RendererAPI
{
public:
    enum class DepthFunc
    {
        Never = 0, Less, Equal, LEqual, Greater, NotEqual, GEqual, Always
    };

    enum class CullMode
    {
        None = 0, Front, Back, FrontAndBack
    };

    enum class BlendFactor
    {
        Zero = 0, One, SrcColor, OneMinusSrcColor, DstColor, OneMinusDstColor, 
        SrcAlpha, OneMinusSrcAlpha, DstAlpha, OneMinusDstAlpha
    };

    enum class API
    {
        None = 0,
        OpenGL = 1,
        // Maybe soon
        Vulkan = 2
    };

public:
    virtual ~RendererAPI() = default;

    virtual void Init() = 0;
    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void SetClearColor(const Color& color) = 0;
    virtual void Clear() = 0;
    
    virtual void SetDepthFunc(DepthFunc func) = 0;
    virtual void SetDepthTest(bool enabled) = 0;
    virtual void SetDepthMask(bool enabled) = 0;
    
    virtual void SetCullMode(CullMode mode) = 0;
    virtual void SetBlendMode(bool enabled) = 0;
    virtual void SetBlendFunc(BlendFactor src, BlendFactor dst) = 0;

    virtual void SetLineWidth(float width) = 0;

    virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
    virtual void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount) = 0;

    static API GetAPI() { return s_API; }
    static std::unique_ptr<RendererAPI> Create();

private:
    static API s_API;
};

} // namespace CHEngine

#endif // CH_RENDERER_API_H
