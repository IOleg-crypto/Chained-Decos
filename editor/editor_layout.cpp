#include "editor_layout.h"
#include "editor/editor_layer.h"
#include "editor/panels/panel.h"
#include "editor_gui.h"
#include "engine/core/application.h"
#include "engine/scene/project.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <fstream>
#include <filesystem>

namespace CHEngine
{

    void EditorLayout::BeginWorkspace()
    {
        static bool dockspaceOpen = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("MainDockSpaceWindow", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();
        ImGui::PopStyleVar(2);

        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
    }

    void EditorLayout::EndWorkspace()
    {
        ImGui::End();
    }

    void EditorLayout::DrawInterface()
    {
        auto &layer = EditorLayer::Get();
        bool isPlaying = (layer.GetSceneState() == SceneState::Play);

        if (ImGui::BeginMenuBar())
        {
            EditorGUI::DrawMenuBar(layer.GetPanels());
            ImGui::EndMenuBar();
        }
    }

    void EditorLayout::ResetLayout()
    {
        // Try to load from default template first
        std::string defaultPath = std::string(PROJECT_ROOT_DIR) + "/imgui_default.ini";
        if (std::filesystem::exists(defaultPath))
        {
            CH_CORE_INFO("EditorLayout: Resetting from template: {}", defaultPath);
            std::ifstream f(defaultPath);
            if (f.is_open())
            {
                std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                ImGui::LoadIniSettingsFromMemory(content.c_str(), content.size());
                return;
            }
        }

        CH_CORE_WARN("EditorLayout: Template not found at {}, using fallback procedural layout", defaultPath);

        // Fallback procedural layout if no template exists
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
        
        ImGuiID main = dockspace_id;
        ImGuiID right = ImGui::DockBuilderSplitNode(main, ImGuiDir_Right, 0.25f, nullptr, &main);
        ImGuiID left  = ImGui::DockBuilderSplitNode(main, ImGuiDir_Left,  0.25f, nullptr, &main);
        ImGuiID down  = ImGui::DockBuilderSplitNode(main, ImGuiDir_Down,  0.30f, nullptr, &main);

        ImGui::DockBuilderDockWindow("Viewport",        main);
        ImGui::DockBuilderDockWindow("Scene Hierarchy", left);
        ImGui::DockBuilderDockWindow("Inspector",       right);
        ImGui::DockBuilderDockWindow("Environment",     right);
        ImGui::DockBuilderDockWindow("Profiler",        right);
        ImGui::DockBuilderDockWindow("Content Browser", down);
        ImGui::DockBuilderDockWindow("Console",         down);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    void EditorLayout::SaveDefaultLayout()
    {
        size_t size = 0;
        const char* settings = ImGui::SaveIniSettingsToMemory(&size);
        if (settings)
        {
            std::string defaultPath = std::string(PROJECT_ROOT_DIR) + "/imgui_default.ini";
            std::ofstream f(defaultPath);
            if (f.is_open())
            {
                f.write(settings, size);
                CH_CORE_INFO("EditorLayout: Saved current layout as default: {}", defaultPath);
            }
            else
            {
                CH_CORE_ERROR("EditorLayout: Failed to open {} for writing!", defaultPath);
            }
        }
    }

} // namespace CHEngine
