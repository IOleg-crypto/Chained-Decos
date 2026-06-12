#ifndef CH_OPENGL_RENDERER_API_H
#define CH_OPENGL_RENDERER_API_H

#include "engine/graphics/api/renderer_api.h"

#include <unordered_map>

namespace Chained
{

class OpenGLRendererAPI : public RendererAPI
{
public:
    virtual void Init() override;
    virtual void SetViewport(int x, int y, int width, int height) override;
    virtual void SetClearColor(const Color& color) override;
    virtual void Clear() override;
    virtual void SetDepthFunc(DepthFunc func) override;
    virtual void SetDepthTest(bool enabled) override;
    virtual void SetDepthMask(bool enabled) override;
    
    virtual void SetCullMode(CullMode mode) override;
    virtual void SetBlendFunc(BlendFactor src, BlendFactor dst) override;
    virtual void SetBlendMode(bool enabled) override;

    virtual bool IsDepthTestEnabled() const override;
    virtual bool IsBlendEnabled() const override;
    virtual bool IsCullFaceEnabled() const override;

    virtual void SetPolygonMode(PolygonMode mode) override;
    virtual void SetPolygonOffset(bool enabled, float factor = 0.0f, float units = 0.0f) override;

    virtual void SetLineWidth(float width) override;

    virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
    virtual void DrawIndexedInstanced(const std::shared_ptr<VertexArray>& vertexArray, uint32_t instanceCount, uint32_t indexCount = 0) override;
    virtual void DrawIndexedLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
    virtual void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount) override;
    virtual void DrawArrays(uint32_t vertexCount) override;
    virtual void DrawArraysInstanced(uint32_t vertexCount, uint32_t instanceCount) override;

    virtual void SetTexture(uint32_t slot, uint32_t textureId, bool isCubemap = false) override;

private:
    float m_ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

    struct RendererStateCache
    {
        uint32_t ActiveShader = 0;
        std::unordered_map<uint32_t, uint32_t> BoundTextures; // Slot -> TextureID
        
        bool DepthTest = false;
        bool DepthMask = true;
        bool Blend = false;
        CullMode Cull = CullMode::None;
        PolygonMode PolyMode = PolygonMode::Fill;

        DepthFunc DepthFunction = DepthFunc::Less;
        BlendFactor SrcBlend = BlendFactor::One;
        BlendFactor DstBlend = BlendFactor::Zero;
    } m_StateCache;
};

} // namespace Chained

#endif // CH_OPENGL_RENDERER_API_H
