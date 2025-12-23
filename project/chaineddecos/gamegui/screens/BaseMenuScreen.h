#ifndef BASE_MENU_SCREEN_H
#define BASE_MENU_SCREEN_H

#include "../interfaces/IMenuScreen.h"
#include <imgui.h>

#include "core/interfaces/IMenu.h"

class BaseMenuScreen : public IMenuScreen
{
public:
    virtual ~BaseMenuScreen() = default;

    void Initialize(IMenu *menu) override
    {
        m_menu = menu;
    }
    virtual void HandleInput() override
    {
    } // Default empty implementation

protected:
    IMenu *m_menu = nullptr;

    // Common UI helpers
    bool RenderActionButton(const char *label, CHEngine::MenuEventType eventType,
                            const ImVec2 &size = ImVec2(0, 0));
    bool RenderBackButton(float width = 0.0f);
    void RenderSectionHeader(const char *title, const char *subtitle = nullptr) const;

    // Type-safe menu accessor for implementations
    class Menu *GetMenu() const;
};

#endif // BASE_MENU_SCREEN_H


