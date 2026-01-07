#ifndef CD_ENGINE_GUI_GUI_BUTTON_H
#define CD_ENGINE_GUI_GUI_BUTTON_H

#include "gui_element.h"

namespace CHEngine
{
class GuiButton : public GuiElement
{
public:
    GuiButton(const std::string &text)
        : m_text(text), m_baseColor(DARKGRAY), m_hoverColor(GRAY), m_pressedColor(LIGHTGRAY),
          m_textColor(WHITE), m_isHovered(false), m_isPressed(false)
    {
        m_size = {200, 50};
    }

    void Update(float deltaTime) override
    {
        Vector2 mousePos = GetMousePosition();
        m_isHovered = CheckCollisionPointRec(mousePos, GetBounds());

        if (m_isHovered)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                m_isPressed = true;
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            {
                if (m_isPressed && m_callback)
                    m_callback();
                m_isPressed = false;
            }
        }
        else
            m_isPressed = false;
    }

    void Render() override
    {
        Color currentBg = m_baseColor;
        if (m_isPressed)
            currentBg = m_pressedColor;
        else if (m_isHovered)
            currentBg = m_hoverColor;

        DrawRectangleRec(GetBounds(), currentBg);
        DrawRectangleLinesEx(GetBounds(), 2, m_textColor);

        int textWidth = MeasureText(m_text.c_str(), 20);
        DrawText(m_text.c_str(), (int)(m_position.x + (m_size.x - textWidth) / 2),
                 (int)(m_position.y + (m_size.y - 20) / 2), 20, m_textColor);
    }

    void SetCallback(std::function<void()> callback)
    {
        m_callback = callback;
    }

private:
    std::string m_text;
    Color m_baseColor, m_hoverColor, m_pressedColor, m_textColor;
    bool m_isHovered, m_isPressed;
    std::function<void()> m_callback;
};
} // namespace CHEngine

#endif
