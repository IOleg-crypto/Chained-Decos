#ifndef CH_TEXTURE_UTILITY_H
#define CH_TEXTURE_UTILITY_H

#include <memory>
#include "engine/graphics/pipeline/renderer_types.h"
#include "engine/graphics/api/texture.h"

namespace CHEngine
{
    class TextureUtility
    {
    public:
        // Converts an equirectangular panorama to a cubemap
        static std::shared_ptr<Texture> GenTextureCubemap(uint32_t shaderId, uint32_t panoramaId, int size, const Mesh& cubeMesh);
    };
}

#endif // CH_TEXTURE_UTILITY_H
