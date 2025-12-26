#include "CreditsScreen.h"
#include "../Menu.h"
#include "../MenuConstants.h"
#include <imgui.h>

void CreditsScreen::Render()
{
    if (!GetMenu())
        return;

    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;

    // Title
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 32.0f);
    ImGui::Text("CREDITS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    float currentY = MenuConstants::TOP_MARGIN + 50;
    const float labelSpacing = 30.0f;
    const float sectionSpacing = 60.0f;

    // Developer section
    ImGui::SetCursorPos(ImVec2(centerX - 100, currentY));
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "DEVELOPER");
    currentY += labelSpacing;
    ImGui::SetCursorPos(ImVec2(centerX - 50, currentY));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "I#Oleg");
    currentY += sectionSpacing;

    // Engine section
    ImGui::SetCursorPos(ImVec2(centerX - 100, currentY));
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "ENGINE");
    currentY += labelSpacing;
    ImGui::SetCursorPos(ImVec2(centerX - 80, currentY));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "raylib + rlImGui");
    currentY += sectionSpacing;

    // UI Design section
    ImGui::SetCursorPos(ImVec2(centerX - 100, currentY));
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "UI DESIGN");
    currentY += labelSpacing;
    ImGui::SetCursorPos(ImVec2(centerX - 80, currentY));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "Modern Interface");

    // Back button
    ImGui::SetCursorPos(ImVec2(centerX - 40, windowSize.y - 60));
    RenderBackButton();
}
