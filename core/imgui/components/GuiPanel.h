#ifndef GUI_PANEL_H
#define GUI_PANEL_H

#include "../core/GuiElement.h"

namespace CHEngine
{
class GuiPanel : public GuiElement
{
public:
    GuiPanel() : m_backgroundColor({40, 40, 40, 200}), m_borderColor(GRAY), m_borderWidth(2)
    {
    }

    void Render() override
    {
        DrawRectangleRec(GetBounds(), m_backgroundColor);
        if (m_borderWidth > 0)
        {
            DrawRectangleLinesEx(GetBounds(), (float)m_borderWidth, m_borderColor);
        }
    }

    void SetBackgroundColor(Color color)
    {
        m_backgroundColor = color;
    }
    void SetBorder(Color color, int width)
    {
        m_borderColor = color;
        m_borderWidth = width;
    }

private:
    Color m_backgroundColor;
    Color m_borderColor;
    int m_borderWidth;
};
} // namespace CHEngine

#endif // GUI_PANEL_H


