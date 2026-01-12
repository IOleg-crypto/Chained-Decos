#ifndef CH_UI_COMPONENT_H
#define CH_UI_COMPONENT_H

#include <functional>
#include <string>
#include <vector>

namespace CHEngine
{
// Base class for game-level UI elements (HUD, Menus)
// Unlike ImGui (Editor UI), this will be designed for the final game.
class UIElement
{
public:
    virtual ~UIElement() = default;
    virtual void OnRender() = 0;
};

struct UIComponent
{
    // Placeholder for game GUI logic
    // This will be used by the Engine's UISystem to draw HUDs.
    bool IsVisible = true;

    // In the future, this could hold a collection of UIElements or a script reference.
    UIComponent() = default;
};

} // namespace CHEngine

#endif // CH_UI_COMPONENT_H
