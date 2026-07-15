#ifndef CH_OPENGL_TEXTURE_H
#define CH_OPENGL_TEXTURE_H

#include "engine/graphics/api/texture.h"
#include <glad/gl.h>

namespace Chained
{

class GLTexture : public Texture
{
public:
    GLTexture(uint32_t width, uint32_t height, TextureFormat format);
    GLTexture(uint32_t size, TextureFormat format); // Cubemap constructor
    GLTexture(uint32_t handle, uint32_t width, uint32_t height);
    virtual ~GLTexture();

    virtual uint32_t GetWidth() const override { return m_Width; }
    virtual uint32_t GetHeight() const override { return m_Height; }
    virtual uint32_t GetNativeHandle() const override { return m_RendererID; }

    virtual void SetData(void* data, uint32_t size) override;
    virtual void Bind(uint32_t slot = 0) const override;
    
    virtual bool IsReady() const override { return m_IsReady; }
    virtual TextureType GetType() const override { return m_Type; }

private:
    uint32_t m_RendererID = 0;
    uint32_t m_Width, m_Height;
    TextureFormat m_Format;
    TextureType m_Type;
    bool m_IsReady = false;
    bool m_OwnsResource = true;

    GLenum m_InternalFormat, m_DataFormat;
};

} // namespace Chained

#endif // CH_OPENGL_TEXTURE_H
