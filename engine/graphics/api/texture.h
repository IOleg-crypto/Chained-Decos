#ifndef CH_TEXTURE_H
#define CH_TEXTURE_H

#include <string>
#include <memory>
#include <cstdint>

namespace Chained
{
enum class TextureFormat
{
    None = 0,
    RGB8,
    RGBA8,
    RGB16F,
    RGBA16F,
    Depth24Stencil8
};

enum class TextureType
{
    Texture2D = 0,
    Cubemap
};

class Texture
{
public:
    virtual ~Texture() = default;

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual uint32_t GetNativeHandle() const = 0;

    virtual void SetData(void* data, uint32_t size) = 0;
    virtual void Bind(uint32_t slot = 0) const = 0;

    virtual bool IsReady() const = 0;
    virtual TextureType GetType() const = 0;

    // GPU resource factory
    static std::shared_ptr<Texture> Create(uint32_t width, uint32_t height,
                                           TextureFormat format = TextureFormat::RGBA8);
    static std::shared_ptr<Texture> CreateCubemap(uint32_t size, TextureFormat format = TextureFormat::RGBA16F);
    static std::shared_ptr<Texture> WrapNative(uint32_t handle, uint32_t width, uint32_t height);

    // Asset loading (Hazel-style)
    static std::shared_ptr<Texture> CreateFromFile(const std::string& path);
};

} // namespace Chained

#endif // CH_TEXTURE_H
