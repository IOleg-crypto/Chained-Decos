#ifndef CH_VISUALS_H
#define CH_VISUALS_H

#include "engine/graphics/render_types.h"
#include "engine/scene/scene.h"


namespace CHEngine
{
class Visuals
{
public:
    static void Init();
    static void Shutdown();

    // High-level scene rendering
    static void DrawScene(Scene *scene, const Camera3D &camera,
                          const DebugRenderFlags *debugFlags = nullptr);

    // Manual rendering overrides if needed
    static void Clear(Color color);
};
} // namespace CHEngine

#endif // CH_VISUALS_H
