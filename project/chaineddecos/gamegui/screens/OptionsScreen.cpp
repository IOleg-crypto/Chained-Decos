#include "OptionsScreen.h"
#include "../Menu.h"
#include "../MenuConstants.h"
#include <imgui.h>

void OptionsScreen::Render()
{
    if (!GetMenu())
        return;

    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;
    const float buttonWidth = 360.0f;
    const float buttonHeight = 60.0f;
    const float spacing = 20.0f;

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 32.0f);
    ImGui::Text("OPTIONS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Options buttons - centered vertically (3 buttons total)
    const int buttonCount = 3;
    const float totalHeight = (buttonCount * buttonHeight) + ((buttonCount - 1) * spacing);
    float startY = centerY - totalHeight / 2.0f;
    float currentY = startY;

    // Video Settings button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Video Settings", CHEngine::MenuEventType::OpenVideoSettings,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        GetMenu()->ShowVideoMenu();
    }
    currentY += buttonHeight + spacing;

    // Audio Settings button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Audio Settings", CHEngine::MenuEventType::OpenAudioSettings,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        GetMenu()->ShowAudioMenu();
    }
    currentY += buttonHeight + spacing;

    // Control Settings button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Control Settings", CHEngine::MenuEventType::OpenControlSettings,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        GetMenu()->ShowControlsMenu();
    }
    currentY += buttonHeight + spacing;

    // Back button
    ImGui::SetCursorPos(ImVec2(80, windowSize.y - 60));
    RenderBackButton();
}
