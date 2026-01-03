#include "menu_bar_panel.h"
#include "editor/logic/editor_project_actions.h"
#include "editor/logic/editor_scene_actions.h"
#include "editor/logic/panel_manager.h"
#include "editor/logic/undo/command_history.h"
#include <imgui.h>

namespace CHEngine
{
MenuBarPanel::MenuBarPanel() : EditorPanel("MenuBar")
{
}

void MenuBarPanel::OnImGuiRender()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                SceneManager::Get().NewScene();
            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                SceneManager::Get().SaveScene();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                Application::Get().Close();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
} // namespace CHEngine
