#include "editor_layout.h"
#include "editor/editor_layer.h"
#include "editor/panels/panel.h"
#include "editor/ui/editor_gui.h"
#include "engine/core/application.h"
#include "engine/scene/project.h"
#include "imgui.h"
#include "imgui_internal.h"

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

        auto eventCallback = [&layer](Event &e) { layer.OnEvent(e); };

        EditorUI::MenuBarState menuState;
        menuState.IsPlaying = isPlaying;
        menuState.Panels = &layer.GetPanels().GetPanels();
        // EditorUI::GUI::DrawToolbar(isPlaying, eventCallback);
        EditorUI::GUI::DrawMenuBar(menuState, eventCallback);

        if (!Project::GetActive())
        {
            // Project selector could be here or handled by project browser
        }
    }

    void EditorLayout::ResetLayout()
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
        ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
        ImGuiID dock_down = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, nullptr, &dock_main_id);

        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
        ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left);
        ImGui::DockBuilderDockWindow("Inspector", dock_right);
        ImGui::DockBuilderDockWindow("Environment", dock_right);
        ImGui::DockBuilderDockWindow("Profiler", dock_right);
        ImGui::DockBuilderDockWindow("Content Browser", dock_down);
        ImGui::DockBuilderDockWindow("Console", dock_down);

        ImGui::DockBuilderFinish(dockspace_id);
    }

} // namespace CHEngine
