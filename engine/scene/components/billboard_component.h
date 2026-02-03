#ifndef CH_BILLBOARD_COMPONENT_H
#define CH_BILLBOARD_COMPONENT_H

#include <string>
#include "raylib.h"
#include "../reflect.h"

namespace CHEngine
{
    struct BillboardComponent
    {
        std::string TexturePath;
        Color Tint = WHITE;
        float Size = 0.2f;
        bool UseDepth = true;

        BillboardComponent() = default;
        BillboardComponent(const BillboardComponent&) = default;
    };

    BEGIN_REFLECT(BillboardComponent)
        PROPERTY(std::string, TexturePath, "Texture")
        PROPERTY(Color, Tint, "Tint")
        PROPERTY(float, Size, "Size")
        PROPERTY(bool, UseDepth, "Use Depth")
    END_REFLECT()
}

#endif // CH_BILLBOARD_COMPONENT_H
