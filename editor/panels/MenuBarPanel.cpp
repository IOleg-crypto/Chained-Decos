#include "MenuBarPanel.h"
#include "editor/logic/EditorProjectActions.h"
#include "editor/logic/EditorSceneActions.h"
#include "editor/logic/PanelManager.h"
#include "editor/logic/undo/CommandHistory.h"
#include <imgui.h>

namespace CHEngine
{
MenuBarPanel::MenuBarPanel(EditorSceneActions *sceneActions, EditorProjectActions *projectActions,
                           CommandHistory *commandHistory, PanelManager *panelManager,
                           bool *showProjectSettings)
    : m_SceneActions(sceneActions), m_ProjectActions(projectActions),
      m_CommandHistory(commandHistory), m_PanelManager(panelManager),
      m_ShowProjectSettings(showProjectSettings)
{
}

void MenuBarPanel::OnImGuiRender()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Project"))
                if (m_ProjectActions)
                    m_ProjectActions->NewProject("NewProject", ".");
            if (ImGui::MenuItem("Open Project..."))
                if (m_ProjectActions)
                    m_ProjectActions->OpenProject("");

            ImGui::Separator();

            if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                if (m_SceneActions)
                    m_SceneActions->OnSceneNew();
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                if (m_SceneActions)
                    m_SceneActions->OnSceneOpen();

            ImGui::Separator();

            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                if (m_SceneActions)
                    m_SceneActions->OnSceneSave();
            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
                if (m_SceneActions)
                    m_SceneActions->OnSceneSaveAs();

            ImGui::Separator();

            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                // Handle exit logic
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false,
                                m_CommandHistory ? m_CommandHistory->CanUndo() : false))
            {
                if (m_CommandHistory)
                    m_CommandHistory->Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false,
                                m_CommandHistory ? m_CommandHistory->CanRedo() : false))
            {
                if (m_CommandHistory)
                    m_CommandHistory->Redo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            static const char *panelNames[] = {"Hierarchy", "Inspector", "Viewport",
                                               "Content Browser", "Console"};
            for (const char *name : panelNames)
            {
                auto panel = m_PanelManager->GetPanel<EditorPanel>(name);
                if (panel)
                {
                    bool visible = panel->IsVisible();
                    if (ImGui::MenuItem(name, nullptr, visible))
                    {
                        panel->SetVisible(!visible);
                    }
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("Project Settings"))
                if (m_ShowProjectSettings)
                    *m_ShowProjectSettings = !(*m_ShowProjectSettings);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
            {
                // Handle about dialog
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}
} // namespace CHEngine
