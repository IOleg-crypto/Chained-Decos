#include "MenuScene.h"
#include "core/Log.h"
#include <imgui.h>
#include <raylib.h>

namespace CHEngine
{

MenuScene::MenuScene() : Scene("MainMenu"), m_PendingAction(MenuAction::None)
{
    CD_CORE_INFO("[MenuScene] Created main menu scene");
}

void MenuScene::OnUpdateRuntime(float deltaTime)
{
    Scene::OnUpdateRuntime(deltaTime);

    if (m_PendingAction == MenuAction::Play)
    {
        m_PendingAction = MenuAction::None;

        // In a real scenario, we'd load the first level here.
        // For debugging, we'll signal the engine to load the game scene.
        CD_CORE_INFO("[MenuScene] Transitioning to Game...");

        // Handle transition via ECSSceneManager
        // Engine::Instance().GetECSSceneManager().LoadScene(...);
    }
    else if (m_PendingAction == MenuAction::Quit)
    {
        // Signal exit
    }
}

void MenuScene::OnRenderRuntime()
{
    // Clear background
    ClearBackground(Color{20, 20, 25, 255});

    // Render ImGui menu
    RenderMenuUI();

    Scene::OnRenderRuntime();
}

void MenuScene::RenderMenuUI()
{
    ImGuiIO &io = ImGui::GetIO();

    // Center window
    ImVec2 windowSize = ImVec2(400, 500);
    ImVec2 windowPos =
        ImVec2((io.DisplaySize.x - windowSize.x) * 0.5f, (io.DisplaySize.y - windowSize.y) * 0.5f);

    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("MainMenu", nullptr, windowFlags);

    // Title
    ImGui::SetCursorPosY(60);
    ImGui::PushFont(io.Fonts->Fonts[0]); // Use default font, can be replaced with custom

    float titleWidth = ImGui::CalcTextSize("CHAINED DECOS").x;
    ImGui::SetCursorPosX((windowSize.x - titleWidth) * 0.5f);
    ImGui::Text("CHAINED DECOS");

    ImGui::PopFont();

    ImGui::SetCursorPosY(150);

    // Center buttons
    float buttonWidth = 200;
    float buttonHeight = 50;
    float buttonSpacing = 20;

    // Play Button
    ImGui::SetCursorPosX((windowSize.x - buttonWidth) * 0.5f);
    if (ImGui::Button("PLAY", ImVec2(buttonWidth, buttonHeight)))
    {
        m_PendingAction = MenuAction::Play;
        CD_CORE_INFO("[MenuScene] Play button clicked");
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + buttonSpacing);

    // Settings Button
    ImGui::SetCursorPosX((windowSize.x - buttonWidth) * 0.5f);
    if (ImGui::Button("SETTINGS", ImVec2(buttonWidth, buttonHeight)))
    {
        m_PendingAction = MenuAction::Settings;
        CD_CORE_INFO("[MenuScene] Settings button clicked");
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + buttonSpacing);

    // Quit Button
    ImGui::SetCursorPosX((windowSize.x - buttonWidth) * 0.5f);
    if (ImGui::Button("QUIT", ImVec2(buttonWidth, buttonHeight)))
    {
        m_PendingAction = MenuAction::Quit;
        CD_CORE_INFO("[MenuScene] Quit button clicked");
    }

    ImGui::End();
}

} // namespace CHEngine
