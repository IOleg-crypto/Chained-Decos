#ifndef CH_UI_RENDERER_H
#define CH_UI_RENDERER_H

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

// singleton renderer for in-world UI canvases and project fonts.
class UIRenderer : public EngineModule
{
public:
    UIRenderer();
    ~UIRenderer() override = default;

    void Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;

    // Draws a UI canvas for the given scene.
    void DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode = false);

    // Resets per-frame button press flags for all ButtonControl/ImageButtonControl components.
    void ResetButtonStates(Scene* scene);

    // Loads fonts required by the current project/UI theme.
    void LoadProjectFonts();

    // Computes the screen-space bounds for a UI entity.
    UIRect GetEntityRect(Scene* scene, Entity entity, const ImVec2& viewportSize, const ImVec2& viewportPos);

    void ResetInputCooldown(int frames = 5) { m_InputCooldownFrames = frames; }
    int  GetInputCooldown() const { return m_InputCooldownFrames; }

    UIFontRegistry&       GetFontRegistry()       { return m_FontRegistry; }
    const UIFontRegistry& GetFontRegistry() const  { return m_FontRegistry; }


private:
    bool RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode);
    std::vector<entt::entity> SortUIEntities(entt::registry& registry);

    UIFontRegistry    m_FontRegistry;
    UILayoutSystem    m_LayoutSystem;
    UIInputSystem     m_InputSystem;
    UIAnimationSystem m_AnimationSystem;

    bool m_Initialized = false;
    int m_InputCooldownFrames = 0;
};

} // namespace Chained

#endif // CH_UI_RENDERER_H
