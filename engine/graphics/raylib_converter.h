#ifndef CH_RAYLIB_CONVERTER_H
#define CH_RAYLIB_CONVERTER_H

#include "engine/scene/project.h" // For TextureFilter
#include "raylib.h"

namespace CHEngine
{
class RaylibConverter
{
public:
    static int ToRaylibFilter(TextureFilter filter)
    {
        switch (filter)
        {
        case TextureFilter::None:           return TEXTURE_FILTER_POINT;
        case TextureFilter::Bilinear:       return TEXTURE_FILTER_BILINEAR;
        case TextureFilter::Trilinear:      return TEXTURE_FILTER_TRILINEAR;
        case TextureFilter::Anisotropic4x:  return TEXTURE_FILTER_ANISOTROPIC_4X;
        case TextureFilter::Anisotropic8x:  return TEXTURE_FILTER_ANISOTROPIC_8X;
        case TextureFilter::Anisotropic16x: return TEXTURE_FILTER_ANISOTROPIC_16X;
        default:                            return TEXTURE_FILTER_BILINEAR;
        }
    }
};
} // namespace CHEngine

#endif // CH_RAYLIB_CONVERTER_H
