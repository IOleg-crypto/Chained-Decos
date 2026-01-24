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

    float toolbarSize = ImGui::GetWindowHeight() - 6.0f;
    float buttonsCount = isPlaying ? 2.0f : 2.0f; // Play/Pause or Stop/Pause
    float totalWidth =
        (toolbarSize * buttonsCount) + (ImGui::GetStyle().ItemSpacing.x * (buttonsCount - 1));

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() * 0.5f) - (totalWidth * 0.5f));
    ImGui::SetCursorPosY(3.0f);

    // Play / Stop Button
    {
        const char *icon = isPlaying ? ICON_FA_CIRCLE_STOP : ICON_FA_PLAY;
        ImVec4 iconColor =
            isPlaying ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
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
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(isPlaying ? "Stop (ESC)" : "Play");
    }

    ImGui::SameLine();

    // Pause Button (Draft - needs engine support for Pause state)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.3f, 1.0f));
        if (ImGui::Button(ICON_FA_PAUSE, ImVec2(toolbarSize, toolbarSize)))
        {
            // TODO: Dispatch Pause Event
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Pause Simulation");
    }

    // Standalone Launch (Only if not playing)
    if (!isPlaying)
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - toolbarSize - 10.0f);
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
