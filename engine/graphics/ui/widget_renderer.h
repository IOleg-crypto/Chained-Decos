#ifndef CH_WIDGET_RENDERER_H
#define CH_WIDGET_RENDERER_H

#include "ui_types.h"
#include "ui_font_registry.h"
#include "ui_layout_system.h"
#include "ui_animation_system.h"
#include "ui_input_system.h"
#include "engine/scene/entity.h"
#include "engine/scene/components.h"
#include "imgui.h"
#include <entt/entt.hpp>
#include <map>

#include <vector>
#include "engine/core/engine_module.h"

namespace Chained
{
class Scene;
class AssetManager;

// Singleton renderer for in-game UI canvases and project fonts.
class WidgetRenderer : public EngineModule
{
public:
    WidgetRenderer();
    ~WidgetRenderer() override = default;

    void Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts);

    // Processes widget input (hover / press hit-testing) for the scene.
    // MUST be called once per frame in the update phase, BEFORE scripts read
    // widget state. Decoupled from DrawCanvas so the per-frame flag reset always
    // runs — even on frames where the canvas is clipped/collapsed and not drawn.
    // Uses the canvas rect cached by the previous DrawCanvas call.
    // When suppressInput is true, flags are reset but no hit-testing is done
    // (used on the first frame after a scene load to drop stale mouse state).
    void ProcessInput(Scene* scene, bool suppressInput = false);

    // Draws a UI canvas for the given scene. Rendering only — input is handled
    // by ProcessInput. Caches the canvas rect for the next frame's ProcessInput.
    void DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode = false);

    // Resets per-frame button press flags for all ButtonControl/ImageButtonControl components.
    void ResetButtonStates(Scene* scene);

    // Loads fonts required by the current project/UI theme.
    void LoadProjectFonts();

    // Computes the screen-space bounds for a UI entity.
    UIRect GetEntityRect(Scene* scene, Entity entity, const ImVec2& viewportSize, const ImVec2& viewportPos);

    UIFontRegistry& GetFontRegistry()
    {
        return m_FontRegistry;
    }
    const UIFontRegistry& GetFontRegistry() const
    {
        return m_FontRegistry;
    }

private:
    bool RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode);
    std::vector<entt::entity> SortUIEntities(entt::registry& registry);

    UIFontRegistry m_FontRegistry;
    UILayoutSystem m_LayoutSystem;
    UIAnimationSystem m_AnimationSystem;

    bool m_Initialized = false;

    // Canvas geometry captured by the last DrawCanvas, reused by ProcessInput on
    // the next frame so input hit-testing does not depend on the render phase
    // actually running this frame.
    ImVec2 m_CanvasPos = {0.0f, 0.0f};
    ImVec2 m_CanvasSize = {0.0f, 0.0f};
    bool m_HasCanvasRect = false;
};

} // namespace Chained

#endif // CH_WIDGET_RENDERER_H
