#ifndef CH_UI_RENDERER_H
#define CH_UI_RENDERER_H

#include "ui_font_registry.h"
#include "engine/core/engine_service.h"
#include "engine/scene/entity.h"
#include "engine/scene/components.h"
#include "imgui.h"
#include "entt/entt.hpp"
#include <map>
#include <vector>

namespace CHEngine
{
class Scene;

// Screen-space rectangle used for UI layout and hit testing.
struct UIRect
{
    float x, y, width, height;
};

// Singleton renderer for in-world UI canvases and project fonts.
class UIRenderer : public EngineService
{
public:
    UIRenderer() =  default;
    virtual ~UIRenderer() override = default;

    // Draws a UI canvas for the given scene.
    void DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode = false);

    // Resets per-frame button press flags for all ButtonControl/ImageButtonControl components.
    static void ResetButtonStates(Scene* scene);

    // Loads fonts required by the current project/UI theme.
    void LoadProjectFonts();

    // Computes the screen-space bounds for a UI entity.
    UIRect GetEntityRect(Entity entity, const ImVec2& viewportSize, const ImVec2& viewportPos);

    UIFontRegistry&       GetFontRegistry()       { return m_FontRegistry; }
    const UIFontRegistry& GetFontRegistry() const  { return m_FontRegistry; }

protected:
    virtual void OnInit() override;
    virtual void OnShutdown() override;

private:
    void   UpdateStyleAnimation(UIStyle& style, bool isHovered, bool isDown, float dt);
    bool   RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode);

    std::vector<entt::entity> SortUIEntities(entt::registry& registry);
    UIRect CalculateEntityRect(Entity entity, const UIRect& canvasRect, std::map<entt::entity, UIRect>& rectCache);

    UIFontRegistry    m_FontRegistry;
    bool m_Initialized = false;
};

} // namespace CHEngine

#endif // CH_UI_RENDERER_H
