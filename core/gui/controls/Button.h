#ifndef GUI_BUTTON_H
#define GUI_BUTTON_H

#include "../GUIContext.h"
#include <string>

namespace gui
{

bool Button(GUIContext &ctx, const std::string &id, const std::string &text, float width = 0,
            float height = 0)
{
    const auto &style = ctx.GetStyle();
    Vector2 cursor = ctx.GetLayoutCursor();

    // Auto-size if 0
    if (width == 0)
        width = 200; // Default width
    if (height == 0)
        height = 40; // Default height

    Rectangle rect = {cursor.x, cursor.y, width, height};

    // Interaction
    bool hovered = ctx.IsMouseHovering(rect);
    bool clicked = false;

    if (hovered)
    {
        ctx.SetHotItem(id);
        if (ctx.IsMouseClicked())
        {
            ctx.SetActiveItem(id);
        }
    }

    // Logic for click (on release of active item while hot)
    if (ctx.IsActive(id))
    {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            if (hovered && ctx.IsHot(id))
            {
                clicked = true;
            }
            // active cleared in EndFrame
        }
    }

    // Render
    Color bgColor = style.colorSecondary;
    if (ctx.IsActive(id))
        bgColor = style.colorPressed;
    else if (hovered)
        bgColor = style.colorHover;

    DrawRectangleRec(rect, bgColor);
    DrawRectangleLinesEx(rect, style.borderWidth, style.colorBorder);

    // Center text
    int textWidth = MeasureText(text.c_str(), (int)style.fontSizeBody);
    int textX = (int)(rect.x + (rect.width - textWidth) / 2);
    int textY = (int)(rect.y + (rect.height - style.fontSizeBody) / 2);

    DrawText(text.c_str(), textX, textY, (int)style.fontSizeBody, style.colorText);

    // Advance
    ctx.AdvanceCursor(width, height);

    return clicked;
}

} // namespace gui

#endif // GUI_BUTTON_H
