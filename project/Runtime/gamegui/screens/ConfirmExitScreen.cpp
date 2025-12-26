#include "ConfirmExitScreen.h"
#include "../Menu.h"
#include "../MenuConstants.h"
#include <imgui.h>

void ConfirmExitScreen::Render()
{
    if (!GetMenu())
        return;

    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;

    // Centered dialog
    const float dialogWidth = 400.0f;
    const float dialogHeight = 200.0f;

    ImGui::SetCursorPos(ImVec2(centerX - dialogWidth * 0.5f, centerY - dialogHeight * 0.5f));
    ImGui::BeginChild("ExitDialog", ImVec2(dialogWidth, dialogHeight), true, ImGuiWindowFlags_None);

    // Title
    ImGui::SetCursorPos(ImVec2(20, 20));
    ImGui::SetWindowFontScale(1.2f);
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "EXIT GAME");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::SetCursorPos(ImVec2(20, 60));
    ImGui::Text("Are you sure you want to quit?");

    const float buttonWidth = 120.0f;
    const float buttonHeight = 40.0f;

    ImGui::SetCursorPos(ImVec2(20, 130));
    if (ImGui::Button("YES, QUIT", ImVec2(buttonWidth, buttonHeight)))
    {
        GetMenu()->DispatchEvent(CHEngine::MenuEventType::ExitGame);
    }

    ImGui::SetCursorPos(ImVec2(160, 130));
    if (ImGui::Button("NO, BACK", ImVec2(buttonWidth, buttonHeight)))
    {
        GetMenu()->ShowMainMenu();
    }

    ImGui::EndChild();
}
