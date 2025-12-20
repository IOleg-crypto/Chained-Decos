#include "MainMenuScreen.h"
#include "../Menu.h"
#include "../MenuConstants.h"
#include <imgui.h>

void MainMenuScreen::Update()
{
    // Implementation moved from Menu::HandleKeyboardNavigation for Main state if needed
}

void MainMenuScreen::Render()
{
    if (!GetMenu())
        return;

    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;
    const float buttonWidth = 360.0f;
    const float buttonHeight = 60.0f;
    const float spacing = 20.0f;

    // Title section
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN - 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
    ImGui::PushFont(nullptr);
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::TITLE_FONT_SIZE) / 32.0f);
    ImGui::Text("CHAINED DECOS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImGui::PopStyleColor();

    // Subtitle
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, MenuConstants::TOP_MARGIN));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::NAME_FONT_SIZE) / 32.0f);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Parkour Adventure");
    ImGui::SetWindowFontScale(1.0f);

    // Menu buttons container
    float startY = MenuConstants::TOP_MARGIN + 100;
    float currentY = startY;

    if (GetMenu()->IsGameInProgress())
    {
        ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
        if (RenderActionButton("Resume Game", ChainedDecos::MenuEventType::ResumeGame,
                               ImVec2(buttonWidth, buttonHeight)))
        {
            // Navigation handled by Menu state via event or direct set
            // For now, m_menu->SetState(MenuState::Resume) if we keep enum for transition
        }
        currentY += buttonHeight + spacing;
    }

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Start Game", ChainedDecos::MenuEventType::None,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        GetMenu()->ShowMapSelection();
    }
    currentY += buttonHeight + spacing;

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Options", ChainedDecos::MenuEventType::OpenOptions,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        GetMenu()->ShowOptionsMenu();
    }
    currentY += buttonHeight + spacing;

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Credits", ChainedDecos::MenuEventType::OpenCredits,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        GetMenu()->ShowCredits();
    }
    currentY += buttonHeight + spacing;

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    if (RenderActionButton("Exit Game", ChainedDecos::MenuEventType::None,
                           ImVec2(buttonWidth, buttonHeight)))
    {
        GetMenu()->ShowConfirmExit();
    }

    // Console toggle hint
    ImGui::SetCursorPos(ImVec2(MenuConstants::MARGIN, windowSize.y - 40));
    ImGui::SetWindowFontScale(static_cast<float>(MenuConstants::INSTRUCTIONS_FONT_SIZE) / 32.0f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                       "[~] Console | [F12] Screenshot | [ESC] Back");
    ImGui::SetWindowFontScale(1.0f);
}
