#include "editor_layout.h"

#include "editor_gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <filesystem>
#include <fstream>

namespace Chained
{

EditorLayout::EditorLayout(EditorPanels& panels)
    : m_Panels(panels)
{
}

void EditorLayout::ResetLayout()
{
    LoadPreset("editor_default_layout.ini");
}

void EditorLayout::SaveDefaultLayout()
{
    SaveCurrent("editor_default_layout.ini");
}

void EditorLayout::LoadPreset(const std::string& filepath)
{
    if (std::filesystem::exists(filepath))
    {
        CH_CORE_INFO("EditorLayout: Loading from preset: {}", filepath);
        std::ifstream f(filepath);
        if (f.is_open())
        {
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            ImGui::LoadIniSettingsFromMemory(content.c_str(), content.size());
            return;
        }
    }
    else
    {
        CH_CORE_WARN("EditorLayout: Preset not found at {}", filepath);
    }
}

void EditorLayout::SaveCurrent(const std::string& filepath)
{
    size_t size = 0;
    const char* settings = ImGui::SaveIniSettingsToMemory(&size);
    if (settings)
    {
        std::ofstream file(filepath);
        if (file.is_open())
        {
            file.write(settings, size);
            CH_CORE_INFO("EditorLayout: Saved current layout to: {}", filepath);
        }
        else
        {
            CH_CORE_ERROR("EditorLayout: Failed to open {} for writing!", filepath);
        }
    }
}

void EditorLayout::OnImGuiRender()
{
    static bool dockspaceOpen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    {
        window_flags |= ImGuiWindowFlags_NoBackground;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("MainDockSpaceWindow", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        m_DockSpaceID = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(m_DockSpaceID, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    EditorGUI::DrawMenuBar(EditorLayer::Get(), m_Panels);

    bool readOnly = EditorLayer::Get().GetSceneState() == SceneState::Play;
    m_Panels.OnImGuiRender(readOnly);

    ImGui::End();
}

} // namespace Chained
