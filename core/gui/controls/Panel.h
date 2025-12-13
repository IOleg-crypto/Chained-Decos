#ifndef GUI_PANEL_H
#define GUI_PANEL_H

#include "../GUIContext.h"
#include <string>

namespace gui
{

// Simple panel helper
// Usage:
//   if (BeginPanel(ctx, "panel1", {100, 100, 200, 200})) {
//       // draw contents
//   }
//   EndPanel(ctx);

bool BeginPanel(GUIContext &ctx, const std::string &id, Rectangle rect)
{
    // Styling
    const auto &style = ctx.GetStyle();

    // Draw background
    // Glassmorphism: semi-transparent background + blur (Raylib doesn't do blur easily without
    // shaders) For now, just semi-transparent
    DrawRectangleRec(rect, style.colorPrimary);

    // Border
    DrawRectangleLinesEx(rect, style.borderWidth, style.colorBorder);

    // Set layout cursor to top-left of panel + padding
    Vector2 contentPos = {rect.x + style.padding, rect.y + style.padding};
    ctx.PushLayoutStart(contentPos);

    return true; // Panel always visible for now
}

void EndPanel(GUIContext &ctx)
{
    ctx.PopLayout();
}

} // namespace gui

#endif // GUI_PANEL_H
