#include "layout.h"

#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "engine/core/log.h"
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
    // Try to load any existing preset
    if (std::filesystem::exists("imgui.ini"))
    {
        LoadPreset("imgui.ini");
    }
    
    // Always trigger a dock rebuild to ensure clean fallback layout
    m_NeedsRebuild = true;
}

void EditorLayout::SaveDefaultLayout()
{
    SaveCurrent("imgui.ini");
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
        
        if (m_NeedsRebuild)
        {
            m_NeedsRebuild = false;

            ImGui::DockBuilderRemoveNode(m_DockSpaceID);
            ImGui::DockBuilderAddNode(m_DockSpaceID, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(m_DockSpaceID, viewport->WorkSize);

            ImGuiID dock_main_id = m_DockSpaceID;
            
            // Layout:
            // dock_left: Scene Hierarchy, World Settings below it
            // dock_right: Inspector, Material Editor
            // dock_bottom: Content Browser, Console
            // Center: Viewport
            
            // Build the layout splits
            ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
            ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
            ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);
            ImGuiID dock_left_bottom = ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Down, 0.5f, nullptr, &dock_left);

            // Assign windows to locations
            ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left);
            ImGui::DockBuilderDockWindow("World Settings", dock_left_bottom);
            
            ImGui::DockBuilderDockWindow("Inspector", dock_right);
            ImGui::DockBuilderDockWindow("Material Editor", dock_right); // grouped with Inspector
            
            ImGui::DockBuilderDockWindow("Content Browser", dock_bottom);
            ImGui::DockBuilderDockWindow("Console", dock_bottom); // grouped with Content Browser

            ImGui::DockBuilderDockWindow("Viewport", dock_main_id); // remainder
            
            ImGui::DockBuilderFinish(m_DockSpaceID);
        }

        ImGui::DockSpace(m_DockSpaceID, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    EditorGUI::DrawMenuBar(EditorLayer::Get(), m_Panels);

    bool readOnly = EditorLayer::Get().GetSceneState() == SceneState::Play;
    m_Panels.OnImGuiRender(readOnly);

    ImGui::End();
}

} // namespace Chained
