#include "toolbar.h"
#include "extras/IconsFontAwesome6.h"
#include <imgui.h>

namespace CH
{
namespace UI
{

void DrawToolbar(bool isPlaying, std::function<void()> onPlay, std::function<void()> onStop)
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
            if (onStop)
                onStop();
        }
        else
        {
            if (onPlay)
                onPlay();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

} // namespace UI
} // namespace CH
