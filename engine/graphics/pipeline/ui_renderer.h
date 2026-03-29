#ifndef CH_UI_RENDERER_H
#define CH_UI_RENDERER_H

#include "ui_font_registry.h"
#include "engine/scene/entity.h"
#include "engine/scene/components.h"
#include "imgui.h"
#include "entt/entt.hpp"
#include <map>
#include <vector>

namespace CHEngine
{
class Scene;

struct UIRect
{
    float x, y, width, height;
};

class UIRenderer
{
public:
    UIRenderer();
    ~UIRenderer();

    static void Init();
    static void Shutdown();
    static UIRenderer& Get();

    void DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode = false);
    void LoadProjectFonts();

    UIRect GetEntityRect(Entity entity, const ImVec2& viewportSize, const ImVec2& viewportPos);

    UIFontRegistry&       GetFontRegistry()       { return m_FontRegistry; }
    const UIFontRegistry& GetFontRegistry() const  { return m_FontRegistry; }

private:
    void   UpdateStyleAnimation(UIStyle& style, bool isHovered, bool isDown, float dt);
    bool   RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode);

    std::vector<entt::entity> SortUIEntities(entt::registry& registry);
    UIRect CalculateEntityRect(Entity entity, const UIRect& canvasRect, std::map<entt::entity, UIRect>& rectCache);

    UIFontRegistry    m_FontRegistry;
    static UIRenderer* s_Instance;
};

} // namespace CHEngine

#endif // CH_UI_RENDERER_H
