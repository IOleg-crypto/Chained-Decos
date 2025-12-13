#ifndef GUI_STYLE_H
#define GUI_STYLE_H

#include <raylib.h>

namespace gui
{

struct GUIStyle
{
    // Colors
    Color colorPrimary = {40, 40, 40, 240};   // Dark background
    Color colorSecondary = {60, 60, 60, 255}; // Slightly lighter
    Color colorAccent = {200, 200, 200, 255}; // Highlight/Accent
    Color colorText = {240, 240, 240, 255};   // Main text
    Color colorTextDisabled = {120, 120, 120, 255};

    // States
    Color colorHover = {70, 70, 70, 255};
    Color colorPressed = {100, 100, 100, 255};
    Color colorBorder = {80, 80, 80, 255};

    // Metrics
    float padding = 10.0f;
    float spacing = 5.0f;
    float cornerRadius = 6.0f;
    float borderWidth = 1.0f;
    float fontSizeTitle = 32.0f;
    float fontSizeBody = 20.0f;
    float fontSizeSmall = 16.0f;

    // Fonts could be stored here if we want to manage them centrally
    // Font fontTitle;
    // Font fontBody;
};

} // namespace gui

#endif // GUI_STYLE_H
