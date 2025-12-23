#ifndef RENDER_COMPONENT_H
#define RENDER_COMPONENT_H

#include <raylib.h>
#include <string>

namespace CHEngine
{
struct RenderComponent
{
    std::string modelName;
    Model *model = nullptr;
    Color tint = WHITE;
    bool visible = true;
    int renderLayer = 0;
    bool castShadows = true;
    bool receiveShadows = true;
    Vector3 offset = {0.0f, 0.0f, 0.0f}; // Visual offset from transform position
};
} // namespace CHEngine

#endif // RENDER_COMPONENT_H
