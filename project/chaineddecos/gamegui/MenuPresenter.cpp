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
    ImGuiStyle &style = ImGui::GetStyle();
    ImGuiIO &io = ImGui::GetIO();

    // Load Gantari Font
    std::string fontPath =
        std::string(PROJECT_ROOT_DIR) + "/resources/font/Gantari/static/Gantari-Regular.ttf";
    if (std::filesystem::exists(fontPath))
    {
        bool fontExists = false;
        for (int i = 0; i < io.Fonts->Fonts.Size; i++)
        {
            const char *name = io.Fonts->Fonts[i]->GetDebugName();
            if (name && std::string(name).find("Gantari") != std::string::npos)
            {
                fontExists = true;
                break;
            }
        }

        if (!fontExists)
        {
            ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 32.0f);
            if (font)
            {
                TraceLog(LOG_INFO, "[MenuPresenter] Loaded custom font: %s", fontPath.c_str());
            }
        }
    }

    // Modern dark theme
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.WindowPadding = ImVec2(16.0f, 16.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);

    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.98f);
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 0.8f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 0.9f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

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
    const float buttonWidth = 360.0f;
    const float buttonHeight = 60.0f;
    const float spacing = 20.0f;

    // Title section
    ImGui::SetCursorPos(ImVec2(50, 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("CHAINED DECOS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::SetCursorPos(ImVec2(50, 100));
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Parkour Adventure");

    float currentY = windowSize.y * 0.3f;

    if (gameInProgress && addResumeButton)
    {
        ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
        RenderActionButton("Resume Game", MenuAction::ResumeGame,
                           ImVec2(buttonWidth, buttonHeight));
        currentY += buttonHeight + spacing;
    }

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    RenderActionButton("Start Game", MenuAction::StartGame, ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    RenderActionButton("Options", MenuAction::OpenOptions, ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    RenderActionButton("Credits", MenuAction::OpenCredits, ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    RenderActionButton("Exit Game", MenuAction::ExitGame, ImVec2(buttonWidth, buttonHeight));

    // Footer hint
    ImGui::SetCursorPos(ImVec2(50, windowSize.y - 40));
    RenderMenuHint("[~] Console | [F12] Screenshot | [ESC] Back");
}

void MenuPresenter::RenderOptionsMenu()
{
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float centerX = windowSize.x * 0.5f;
    const float centerY = windowSize.y * 0.5f;
    const float buttonWidth = 360.0f;
    const float buttonHeight = 60.0f;
    const float spacing = 20.0f;

    // Title
    ImGui::SetCursorPos(ImVec2(50, 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("OPTIONS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    const int buttonCount = 3;
    const float totalHeight = (buttonCount * buttonHeight) + ((buttonCount - 1) * spacing);
    float currentY = centerY - totalHeight / 2.0f;

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    RenderActionButton("Video Settings", MenuAction::OpenVideoMode,
                       ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    RenderActionButton("Audio Settings", MenuAction::OpenAudio, ImVec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + spacing;

    ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2, currentY));
    RenderActionButton("Control Settings", MenuAction::OpenControls,
                       ImVec2(buttonWidth, buttonHeight));

    // Back button
    ImGui::SetCursorPos(ImVec2(80, windowSize.y - 60));
    RenderBackButton();
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

    ImGui::SetNextWindowPos(ImVec2(windowSize.x / 2 - 200, windowSize.y / 2 - 150));
    ImGui::SetNextWindowSize(ImVec2(400, 300));

    ImGui::Begin("Exit Confirmation", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::SetCursorPos(ImVec2(150, 40));
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "EXIT GAME?");

    ImGui::SetCursorPos(ImVec2(80, 200));
    if (ImGui::Button("YES", ImVec2(80, 40)))
    {
        if (m_actionCallback)
            m_actionCallback(MenuAction::ExitGame);
    }

    ImGui::SameLine();
    ImGui::SetCursorPos(ImVec2(240, 200));
    if (ImGui::Button("NO", ImVec2(80, 40)))
    {
        if (m_actionCallback)
            m_actionCallback(MenuAction::BackToMainMenu);
    }

    ImGui::SetCursorPos(ImVec2(120, 260));
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, 1.0f), "Y/ENTER = Yes    N/ESC = No");

    ImGui::End();
}

void MenuPresenter::RenderConsoleOverlay(ConsoleManager *consoleManager)
{
    if (consoleManager)
    {
        consoleManager->RenderConsole();
    }
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
