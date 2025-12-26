#include "MenuBarPanel.h"
#include <imgui.h>

namespace CHEngine
{
void MenuBarPanel::OnImGuiRender(const PanelVisibility &visibility,
                                 const MenuBarCallbacks &callbacks)
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
                if (callbacks.OnNew)
                    callbacks.OnNew();
            if (ImGui::MenuItem("Open...", "Ctrl+O"))
                if (callbacks.OnOpen)
                    callbacks.OnOpen();
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                if (callbacks.OnSave)
                    callbacks.OnSave();
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                if (callbacks.OnSaveAs)
                    callbacks.OnSaveAs();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                if (callbacks.OnExit)
                    callbacks.OnExit();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false,
                                callbacks.CanUndo ? callbacks.CanUndo() : false))
            {
                if (callbacks.OnUndo)
                    callbacks.OnUndo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false,
                                callbacks.CanRedo ? callbacks.CanRedo() : false))
            {
                if (callbacks.OnRedo)
                    callbacks.OnRedo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Hierarchy", nullptr, visibility.Hierarchy))
                if (callbacks.TogglePanel)
                    callbacks.TogglePanel("Hierarchy");
            if (ImGui::MenuItem("Inspector", nullptr, visibility.Inspector))
                if (callbacks.TogglePanel)
                    callbacks.TogglePanel("Inspector");
            if (ImGui::MenuItem("Viewport", nullptr, visibility.Viewport))
                if (callbacks.TogglePanel)
                    callbacks.TogglePanel("Viewport");
            if (ImGui::MenuItem("Asset Browser", nullptr, visibility.AssetBrowser))
                if (callbacks.TogglePanel)
                    callbacks.TogglePanel("Asset Browser");
            if (ImGui::MenuItem("Console", nullptr, visibility.Console))
                if (callbacks.TogglePanel)
                    callbacks.TogglePanel("Console");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("Project Settings"))
                if (callbacks.OnShowProjectSettings)
                    callbacks.OnShowProjectSettings();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
            {
                if (callbacks.OnAbout)
                    callbacks.OnAbout();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}
} // namespace CHEngine
