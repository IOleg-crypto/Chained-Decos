#ifndef CH_TEXTURE_UTILITY_H
#define CH_TEXTURE_UTILITY_H

#include "engine/graphics/api/shader.h"
#include "engine/graphics/api/texture.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include <memory>

namespace Chained
{
namespace TextureUtility
{
// Flips image data vertically in memory. Thread-safe if data is unique.
void FlipImageVertically(void* data, int width, int height, int channels, bool isHDR);
// Converts an equirectangular panorama to a cubemap
std::shared_ptr<Texture> GenTextureCubemap(const std::shared_ptr<Shader>& shader, uint32_t panoramaId, int size,
                                           const Mesh& cubeMesh);
}; // namespace TextureUtility
} // namespace Chained

#endif // CH_TEXTURE_UTILITY_H