//
// EditorPanelManager.cpp - Implementation of panel manager
//

#include "EditorPanelManager.h"
#include <imgui.h>
#include <imgui_internal.h>

EditorPanelManager::EditorPanelManager(IEditor *editor) : m_editor(editor)
{
}

IEditorPanel *EditorPanelManager::GetPanel(const std::string &name)
{
    auto it = m_panels.find(name);
    return (it != m_panels.end()) ? it->second.get() : nullptr;
}

void EditorPanelManager::Update(float deltaTime)
{
    for (const auto &name : m_panelOrder)
    {
        auto *panel = GetPanel(name);
        if (panel && panel->IsVisible())
        {
            panel->Update(deltaTime);
        }
    }
}

void EditorPanelManager::Render()
{
    // Use the dockspace ID from the viewport rendering
    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");

    if (m_needsLayoutReset)
    {
        SetupDefaultLayout(dockspaceId);
        m_needsLayoutReset = false;
    }

    for (const auto &name : m_panelOrder)
    {
        auto *panel = GetPanel(name);
        if (panel && panel->IsVisible())
        {
            panel->Render();
        }
    }
}

void EditorPanelManager::SetPanelVisible(const std::string &name, bool visible)
{
    if (auto *panel = GetPanel(name))
    {
        panel->SetVisible(visible);
    }
}

bool EditorPanelManager::IsPanelVisible(const std::string &name) const
{
    auto it = m_panels.find(name);
    return (it != m_panels.end()) ? it->second->IsVisible() : false;
}

void EditorPanelManager::TogglePanelVisibility(const std::string &name)
{
    if (auto *panel = GetPanel(name))
    {
        panel->SetVisible(!panel->IsVisible());
    }
}

void EditorPanelManager::SetAllPanelsVisible(bool visible)
{
    for (auto &pair : m_panels)
    {
        pair.second->SetVisible(visible);
    }
}

bool EditorPanelManager::IsAnyPanelVisible() const
{
    for (const auto &pair : m_panels)
    {
        if (pair.second->IsVisible())
            return true;
    }
    return false;
}

void EditorPanelManager::RenderViewMenu()
{
    if (ImGui::BeginMenu("View"))
    {
        for (const auto &name : m_panelOrder)
        {
            auto *panel = GetPanel(name);
            if (panel)
            {
                bool visible = panel->IsVisible();
                if (ImGui::MenuItem(name.c_str(), nullptr, &visible))
                {
                    panel->SetVisible(visible);
                }
            }
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Reset Layout"))
        {
            ResetLayout();
        }

        ImGui::EndMenu();
    }
}

void EditorPanelManager::SetupDefaultLayout(unsigned int dockspaceId)
{
    ImGuiContext &g = *GImGui;
    ImGui::DockBuilderRemoveNode(dockspaceId); // Clear existing layout
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, g.IO.DisplaySize);

    ImGuiID dock_main_id = dockspaceId;

    // 1. Split top for Toolbar (spans full width)
    ImGuiID dock_id_toolbar =
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.04f, nullptr, &dock_main_id);

    // Remove tab bar from Toolbar node for a professional look
    ImGuiDockNode *toolbar_node = ImGui::DockBuilderGetNode(dock_id_toolbar);
    if (toolbar_node)
    {
        toolbar_node->LocalFlags |=
            ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoWindowMenuButton |
            ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoDockingOverMe;
    }

    // 2. Split right for Inspector (takes full height of remaining space, below toolbar)
    ImGuiID dock_id_right =
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);

    // 3. Split bottom for Assets (takes bottom of center-left area)
    ImGuiID dock_id_bottom =
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.3f, nullptr, &dock_main_id);

    // 4. Split left for Hierarchy (takes left of center area)
    ImGuiID dock_id_left =
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);

    // Remaining is Viewport
    ImGuiID dock_id_viewport = dock_main_id;

    // Assign panels to slots
    ImGui::DockBuilderDockWindow("Toolbar", dock_id_toolbar);
    ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left);
    ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
    ImGui::DockBuilderDockWindow("Asset Browser", dock_id_bottom);
    ImGui::DockBuilderDockWindow("Console", dock_id_bottom); // Console tabs with Asset Browser
    ImGui::DockBuilderDockWindow("Viewport", dock_id_viewport);

    ImGui::DockBuilderFinish(dockspaceId);
}
