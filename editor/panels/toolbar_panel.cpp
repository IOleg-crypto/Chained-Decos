
#include "toolbar_panel.h"
#include "editor/fa5_compat.h"
#include "editor/logic/editor_scene_actions.h"
#include "editor/logic/scene_simulation_manager.h"
#include "imgui_internal.h"
#include <imgui.h>

namespace CHEngine
{

ToolbarPanel::ToolbarPanel(EditorSceneActions *sceneActions, SceneSimulationManager *simulation,
                           Tool *activeTool)
    : m_SceneActions(sceneActions), m_SimulationManager(simulation), m_ActiveTool(activeTool)
{
}

void ToolbarPanel::OnImGuiRender()
{
    // Local aliases for readability (Hazel-style)
    SceneState sceneState = m_SimulationManager->GetSceneState();
    RuntimeMode runtimeMode = m_SimulationManager->GetRuntimeMode();
    Tool activeTool = *m_ActiveTool;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

    auto &colors = ImGui::GetStyle().Colors;
    const auto &buttonHovered = colors[ImGuiCol_ButtonHovered];
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
    const auto &buttonActive = colors[ImGuiCol_ButtonActive];
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 32)); // Fixed height for toolbar

    ImGui::Begin("##toolbar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize);

    float size = ImGui::GetWindowHeight() - 4.0f;

    // Center the controls
    float totalControlsWidth = (size * 6.0f); // approx for 2 play buttons + 4 tool buttons
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x * 0.5f - (totalControlsWidth * 0.5f));

    // 1. Scene State Controls
    bool isEdit = (sceneState == SceneState::Edit);

    // Play/Stop Button
    const char *playIcon = isEdit ? ICON_FA_PLAY : ICON_FA_STOP;
    ImColor playColor = isEdit ? ImColor(0.3f, 0.9f, 0.3f, 1.0f) : ImColor(0.9f, 0.3f, 0.3f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)playColor);
    if (ImGui::Button(playIcon, ImVec2(size, size)))
    {
        if (isEdit)
            m_SceneActions->OnScenePlay();
        else
            m_SceneActions->OnSceneStop();
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // 2. Tool Selection
    const char *toolIcons[] = {ICON_FA_MOUSE_POINTER, ICON_FA_ARROWS_ALT, ICON_FA_SYNC,
                               ICON_FA_EXPAND_ARROWS_ALT};
    for (int i = 0; i < 4; i++)
    {
        Tool tool = (Tool)i;
        ImGui::PushID(i);
        bool active = (*m_ActiveTool == tool);
        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button, colors[ImGuiCol_ButtonActive]);

        if (ImGui::Button(toolIcons[i], ImVec2(size, size)))
            *m_ActiveTool = tool;

        if (active)
            ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::SameLine();
    }

    ImGui::End();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}
} // namespace CHEngine
