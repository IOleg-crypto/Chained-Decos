#include "MenuPresenter.h"
#include "Menu.h"
#include <filesystem>
#include <raylib.h>

void MenuPresenter::SetActionCallback(ActionCallback callback)
{
    m_actionCallback = std::move(callback);
}

void MenuPresenter::SetupStyle()
{
    // Get ImGui's default style and customize it
    ImGuiStyle &style = ImGui::GetStyle();
    ImGuiIO &io = ImGui::GetIO();

    // Load Gantari Font
    std::string fontPath =
        std::string(PROJECT_ROOT_DIR) + "/resources/font/Gantari/static/Gantari-Regular.ttf";
    if (std::filesystem::exists(fontPath))
    {
        // Increase base font size for sharper text (was 20.0f)
        ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 32.0f);
        if (font)
        {
            TraceLog(LOG_INFO, "[Menu] Loaded custom font: %s", fontPath.c_str());
            // Important: Notify rlImGui to rebuild the font texture
            // Assuming rlImGuiReloadFonts() or similar is available, or we just rely on standard
            // flow. Since we don't have direct access to rlImGui internal reload easily without
            // including it, we'll see if it works. Standard ImGui requires texture rebuild. If
            // rlImGui is used, we usually need to call rlImGuiReloadFonts(). Let's try to verify if
            // we can include it.
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "[Menu] Custom font not found: %s", fontPath.c_str());
    }

    // Window styling
    style.WindowRounding = 8.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowPadding = ImVec2(20, 20);

    // Frame styling
    style.FrameRounding = 4.0f;
    style.FramePadding = ImVec2(10, 8);

    // Button styling
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);

    // Colors - Dark theme with purple accents
    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.15f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.4f, 0.3f, 0.6f, 0.5f);
    colors[ImGuiCol_Button] = ImVec4(0.3f, 0.2f, 0.5f, 0.8f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.4f, 0.3f, 0.6f, 0.9f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.5f, 0.4f, 0.7f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.95f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.2f, 0.8f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.2f, 0.3f, 0.9f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.0f);

    m_customStyle = style;
}

bool MenuPresenter::RenderActionButton(const char *label, MenuAction action, const ImVec2 &size)
{
    // Enhanced button styling
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    bool clicked = ImGui::Button(label, size);

    ImGui::PopStyleColor(4);

    if (clicked && action != MenuAction::None && m_actionCallback)
    {
        m_actionCallback(action);
    }

    return clicked;
}

bool MenuPresenter::RenderBackButton(float width)
{
    ImVec2 buttonSize(width > 0 ? width : 120.0f, 40.0f);
    return RenderActionButton("Back", MenuAction::BackToMainMenu, buttonSize);
}

void MenuPresenter::RenderSectionHeader(const char *title, const char *subtitle) const
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextUnformatted(title);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    if (subtitle)
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", subtitle);
    }
}

void MenuPresenter::RenderMenuHint(const char *text) const
{
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", text);
}

void MenuPresenter::RenderMainMenu(bool gameInProgress, bool addResumeButton)
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float buttonWidth = 280.0f;
    const float buttonHeight = 50.0f;
    const float spacing = 15.0f;

    float currentY = windowSize.y * 0.3f;

    // Title
    ImGui::SetCursorPos(ImVec2(centerX - 150.0f, currentY - 80.0f));
    RenderSectionHeader("CHAINED DECOS", nullptr);

    // Resume button (only if game in progress)
    if (gameInProgress && addResumeButton)
    {
        ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, currentY));
        RenderActionButton("Resume Game", MenuAction::ResumeGame,
                           ImVec2(buttonWidth, buttonHeight));
        currentY += buttonHeight + spacing;
    }

    // Play button
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, currentY));
    RenderActionButton("Play", MenuAction::StartGame, ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    // Options
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, currentY));
    RenderActionButton("Options", MenuAction::OpenOptions, ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    // Credits
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, currentY));
    RenderActionButton("Credits", MenuAction::OpenCredits, ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    // Exit
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, currentY));
    RenderActionButton("Exit", MenuAction::ExitGame, ImVec2(buttonWidth, buttonHeight));
}

void MenuPresenter::RenderOptionsMenu()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float buttonWidth = 280.0f;
    const float buttonHeight = 50.0f;
    const float spacing = 15.0f;

    float currentY = windowSize.y * 0.25f;

    // Title
    ImGui::SetCursorPos(ImVec2(centerX - 100.0f, currentY - 60.0f));
    RenderSectionHeader("OPTIONS", nullptr);

    // Video
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, currentY));
    RenderActionButton("Video", MenuAction::OpenVideoMode, ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    // Audio
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, currentY));
    RenderActionButton("Audio", MenuAction::OpenAudio, ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    // Controls
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, currentY));
    RenderActionButton("Controls", MenuAction::OpenControls, ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    // Back
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, windowSize.y - 80.0f));
    RenderBackButton(buttonWidth);
}

void MenuPresenter::RenderGameModeMenu()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float buttonWidth = 280.0f;
    const float buttonHeight = 50.0f;
    const float spacing = 15.0f;

    float currentY = windowSize.y * 0.3f;

    // Title
    ImGui::SetCursorPos(ImVec2(centerX - 120.0f, currentY - 60.0f));
    RenderSectionHeader("SELECT MODE", nullptr);

    // Single Player
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, currentY));
    RenderActionButton("Single Player", MenuAction::SinglePlayer,
                       ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    // Multiplayer (coming soon)
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, currentY));
    ImGui::BeginDisabled(true);
    RenderActionButton("Multiplayer (Coming Soon)", MenuAction::MultiPlayer,
                       ImVec2(buttonWidth, buttonHeight));
    ImGui::EndDisabled();
    currentY += buttonHeight + spacing;

    // Back
    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth * 0.5f, windowSize.y - 80.0f));
    RenderBackButton(buttonWidth);
}

void MenuPresenter::RenderCreditsScreen()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;

    // Title
    ImGui::SetCursorPos(ImVec2(centerX - 80.0f, 100.0f));
    RenderSectionHeader("CREDITS", nullptr);

    // Content
    ImGui::SetCursorPos(ImVec2(centerX - 150.0f, 180.0f));
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.9f, 1.0f), "Game developed by [Developer Name]");

    ImGui::SetCursorPos(ImVec2(centerX - 150.0f, 220.0f));
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "Special thanks to:");

    ImGui::SetCursorPos(ImVec2(centerX - 130.0f, 250.0f));
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "- Raylib team");

    ImGui::SetCursorPos(ImVec2(centerX - 130.0f, 275.0f));
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "- Dear ImGui team");

    // Back button
    ImGui::SetCursorPos(ImVec2(centerX - 60.0f, windowSize.y - 80.0f));
    RenderBackButton(120.0f);
}

void MenuPresenter::RenderModsScreen()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;

    // Title
    ImGui::SetCursorPos(ImVec2(centerX - 50.0f, 100.0f));
    RenderSectionHeader("MODS", nullptr);

    // Content
    ImGui::SetCursorPos(ImVec2(centerX - 100.0f, 180.0f));
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Coming soon...");

    // Back button
    ImGui::SetCursorPos(ImVec2(centerX - 60.0f, windowSize.y - 80.0f));
    RenderBackButton(120.0f);
}

void MenuPresenter::RenderConfirmExitDialog()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;

    // Dialog background
    ImGui::SetCursorPos(ImVec2(centerX - 150.0f, centerY - 60.0f));
    ImGui::BeginChild("ExitDialog", ImVec2(300, 120), true);

    ImGui::SetCursorPosX(60.0f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Exit to Desktop?");

    ImGui::SetCursorPosY(50.0f);
    ImGui::SetCursorPosX(30.0f);

    if (ImGui::Button("Yes", ImVec2(100, 40)))
    {
        if (m_actionCallback)
            m_actionCallback(MenuAction::ExitGame);
    }

    ImGui::SameLine(140.0f);

    if (ImGui::Button("No", ImVec2(100, 40)))
    {
        if (m_actionCallback)
            m_actionCallback(MenuAction::BackToMainMenu);
    }

    ImGui::EndChild();
}

const char *MenuPresenter::GetStateTitle(MenuState state)
{
    switch (state)
    {
    case MenuState::Main:
        return "Main Menu";
    case MenuState::GameMode:
        return "Game Mode";
    case MenuState::Options:
        return "Options";
    case MenuState::Video:
        return "Video Settings";
    case MenuState::Audio:
        return "Audio Settings";
    case MenuState::Controls:
        return "Control Settings";
    case MenuState::MapSelection:
        return "Select Map";
    case MenuState::Credits:
        return "Credits";
    case MenuState::Mods:
        return "Mods";
    case MenuState::ConfirmExit:
        return "Confirm Exit";
    default:
        return "Menu";
    }
}
