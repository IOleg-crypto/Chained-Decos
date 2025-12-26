#ifndef BASE_MENU_SCREEN_H
#define BASE_MENU_SCREEN_H

#include "../interfaces/IMenuScreen.h"
#include <imgui.h>
#include <string>

class Menu;

class BaseMenuScreen : public IMenuScreen
{
public:
    BaseMenuScreen() = default;
    virtual ~BaseMenuScreen() = default;

    void Initialize(IMenu *menu) override;
    void OnEvent(CHEngine::Event &e) override;

    void Update() override
    {
    }
    void Render() override
    {
    }
    void HandleInput() override
    {
    }

    const char *GetTitle() const override
    {
        return "Base";
    }

protected:
    Menu *GetMenu() const;

    // UI Helpers that delegate back to menu
    bool RenderActionButton(const char *label, CHEngine::MenuEventType eventType,
                            const ImVec2 &size = ImVec2(0, 0));
    bool RenderBackButton(float width = 0.0f);
    void RenderSectionHeader(const char *title, const char *subtitle = nullptr) const;
    void RenderMenuHint(const char *text) const;

private:
    IMenu *m_menu = nullptr;
};

#endif // BASE_MENU_SCREEN_H
