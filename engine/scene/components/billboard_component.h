#ifndef CH_BILLBOARD_COMPONENT_H
#define CH_BILLBOARD_COMPONENT_H

#include <string>
#include "raylib.h"

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
}

#endif // CH_BILLBOARD_COMPONENT_H
