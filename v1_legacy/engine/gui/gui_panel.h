#ifndef CD_ENGINE_GUI_GUI_PANEL_H
#define CD_ENGINE_GUI_GUI_PANEL_H

#include "gui_element.h"

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
            DrawRectangleLinesEx(GetBounds(), (float)m_borderWidth, m_borderColor);
    }

    void SetBackgroundColor(Color color)
    {
        m_backgroundColor = color;
    }

private:
    Color m_backgroundColor, m_borderColor;
    int m_borderWidth;
};
} // namespace CHEngine

#endif
