#ifndef RENDER_COMPONENT_H
#define RENDER_COMPONENT_H

#include <raylib.h>
#include <string>

struct RenderComponent
{
    std::string modelName;
    Model *model = nullptr;
    Color tint = WHITE;
    bool visible = true;
    int renderLayer = 0; // Для сортування (0 = default, higher = later)
    bool castShadows = true;
    bool receiveShadows = true;
};

#endif // RENDER_COMPONENT_H
