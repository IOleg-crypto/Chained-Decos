#ifndef CH_UI_RENDERER_H
#define CH_UI_RENDERER_H

#include "engine/scene/scene.h"
#include "imgui.h"

namespace CHEngine
{
    class UIRenderer
    {
    public:
        // Main entry point for drawing UI for a scene.
        static void DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode = false);

    private:
        // Helper scope for UI styling
        struct UIStyleScope;
    };
}

#endif // CH_UI_RENDERER_H
