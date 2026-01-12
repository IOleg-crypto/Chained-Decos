#include "toolbar.h"
#include "extras/IconsFontAwesome6.h"
#include <imgui.h>

namespace CHEngine
{
namespace UI
{

void DrawToolbar(bool isPlaying, const EventCallbackFn &callback)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    auto &styleColors = ImGui::GetStyle().Colors;
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(styleColors[ImGuiCol_ButtonHovered].x,
                                 styleColors[ImGuiCol_ButtonHovered].y,
                                 styleColors[ImGuiCol_ButtonHovered].z, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(styleColors[ImGuiCol_ButtonActive].x,
                                 styleColors[ImGuiCol_ButtonActive].y,
                                 styleColors[ImGuiCol_ButtonActive].z, 0.5f));

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 32));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::Begin("##toolbar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav);

    float toolbarSize = ImGui::GetWindowHeight() - 4.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (toolbarSize * 0.5f));

    const char *icon = isPlaying ? ICON_FA_CIRCLE_STOP : ICON_FA_PLAY;
    if (ImGui::Button(icon, ImVec2(toolbarSize, toolbarSize)))
    {
        if (isPlaying)
        {
            SceneStopEvent e;
            callback(e);
        }
        else
        {
            ScenePlayEvent e;
            callback(e);
        }
    }

    if (!isPlaying)
    {
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ROCKET, ImVec2(toolbarSize, toolbarSize)))
        {
            AppLaunchRuntimeEvent e;
            callback(e);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Launch Standalone Runtime");
    }

    if (isPlaying)
    {
        ImGui::SameLine();
        float textWidth = ImGui::CalcTextSize("Press ESC to Exit").x;
        float padding = 20.0f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - textWidth - padding);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (toolbarSize * 0.2f));
        ImGui::Text("Press ESC to Exit");
        ImGui::PopStyleColor();
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

} // namespace UI
} // namespace CHEngine
