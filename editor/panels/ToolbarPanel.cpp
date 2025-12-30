#include "ToolbarPanel.h"
#include "editor/utils/IconsFontAwesome5.h"
#include <imgui.h>

namespace CHEngine
{
void ToolbarPanel::OnImGuiRender(SceneState sceneState, RuntimeMode runtimeMode, Tool activeTool,
                                 const std::function<void()> &onPlay,
                                 const std::function<void()> &onStop,
                                 const std::function<void()> &onNew,
                                 const std::function<void()> &onSave,
                                 const std::function<void(Tool)> &onToolChange,
                                 const std::function<void(RuntimeMode)> &onRuntimeModeChange)
{
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
    float totalControlsWidth = (size * 2.0f) + 120.0f + 10.0f; // 2 buttons + combo + spacing
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x * 0.5f - (totalControlsWidth * 0.5f));

    // 1. Scene State Controls
    bool isEdit = (sceneState == SceneState::Edit);

    // Play Button
    ImGui::BeginDisabled(!isEdit);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1.0f)); // Bright green
    if (ImGui::Button(ICON_FA_PLAY, ImVec2(size, size)))
        onPlay();
    ImGui::PopStyleColor();
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Stop Button
    ImGui::BeginDisabled(isEdit);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f)); // Bright red
    if (ImGui::Button(ICON_FA_STOP, ImVec2(size, size)))
        onStop();
    ImGui::PopStyleColor();
    ImGui::EndDisabled();

    // Runtime Mode Selection
    ImGui::SameLine();
    ImGui::BeginDisabled(!isEdit);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
    ImGui::PushItemWidth(120.0f);
    const char *modes[] = {"Embedded", "Standalone"};
    int currentMode = (int)runtimeMode;
    if (ImGui::Combo("##runtime_mode", &currentMode, modes, IM_ARRAYSIZE(modes)))
    {
        onRuntimeModeChange((RuntimeMode)currentMode);
    }
    ImGui::PopItemWidth();
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::SetCursorPosX(10.0f); // Move file controls to the left

    // 2. File Controls
    if (ImGui::Button(ICON_FA_FILE, ImVec2(size + 10, size)))
        onNew();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("New Scene");

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_SAVE, ImVec2(size + 10, size)))
        onSave();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Save Scene");

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // 3. Tool Selection
    auto toolButton = [&](Tool tool, const char *icon, const char *tooltip)
    {
        bool selected = (activeTool == tool);
        if (selected)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 0.6f));

        if (ImGui::Button(icon, ImVec2(size + 5, size)))
            onToolChange(tool);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", tooltip);

        if (selected)
            ImGui::PopStyleColor();
        ImGui::SameLine();
    };

    toolButton(Tool::SELECT, ICON_FA_MOUSE_POINTER, "Select (Q)");
    toolButton(Tool::MOVE, ICON_FA_ARROWS_ALT, "Move (W)");
    toolButton(Tool::ROTATE, ICON_FA_SYNC, "Rotate (E)");
    toolButton(Tool::SCALE, ICON_FA_EXPAND_ARROWS_ALT, "Scale (R)");

    ImGui::End();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}
} // namespace CHEngine
