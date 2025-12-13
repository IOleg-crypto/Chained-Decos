#ifndef GUI_LABEL_H
#define GUI_LABEL_H

#include "../GUIContext.h"
#include <string>

namespace gui
{

void Label(GUIContext &ctx, const std::string &text)
{
    const auto &style = ctx.GetStyle();
    Vector2 cursor = ctx.GetLayoutCursor();

    DrawText(text.c_str(), (int)cursor.x, (int)cursor.y, (int)style.fontSizeBody, style.colorText);

    // Advance cursor
    // Measure text to know height?
    // For simplicity, fixed line height based on font size + spacing
    ctx.AdvanceCursor(MeasureText(text.c_str(), (int)style.fontSizeBody), style.fontSizeBody);
}

void Title(GUIContext &ctx, const std::string &text)
{
    const auto &style = ctx.GetStyle();
    Vector2 cursor = ctx.GetLayoutCursor();

    DrawText(text.c_str(), (int)cursor.x, (int)cursor.y, (int)style.fontSizeTitle,
             style.colorAccent);

    ctx.AdvanceCursor(MeasureText(text.c_str(), (int)style.fontSizeTitle), style.fontSizeTitle);
}

} // namespace gui

#endif // GUI_LABEL_H
