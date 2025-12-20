//
// HierarchyPanel.cpp - Implementation of scene hierarchy panel
//

#include "HierarchyPanel.h"
#include "editor/IEditor.h"
#include <imgui.h>

HierarchyPanel::HierarchyPanel(IEditor *editor) : m_editor(editor)
{
}

void HierarchyPanel::Render()
{
    if (!m_visible)
        return;

    ImGui::Begin("Hierarchy", &m_visible);

    if (!m_editor)
    {
        ImGui::Text("No editor instance");
        ImGui::End();
        return;
    }

    // Get objects from game map
    auto &gameMap = m_editor->GetGameMap();
    auto &objects = gameMap.GetMapObjectsMutable();

    // Header with object count
    ImGui::Text("Objects: %zu", objects.size());
    ImGui::Separator();

    // Add object button
    if (ImGui::Button("+ Add Object"))
    {
        ImGui::OpenPopup("AddObjectPopup");
    }

    if (ImGui::BeginPopup("AddObjectPopup"))
    {
        if (ImGui::MenuItem("Cube"))
        {
            m_editor->CreateDefaultObject(MapObjectType::CUBE);
        }
        if (ImGui::MenuItem("Sphere"))
        {
            m_editor->CreateDefaultObject(MapObjectType::SPHERE);
        }
        if (ImGui::MenuItem("Cylinder"))
        {
            m_editor->CreateDefaultObject(MapObjectType::CYLINDER);
        }
        if (ImGui::MenuItem("Plane"))
        {
            m_editor->CreateDefaultObject(MapObjectType::PLANE);
        }
        if (ImGui::MenuItem("Model"))
        {
            m_editor->CreateDefaultObject(MapObjectType::MODEL);
        }
        if (ImGui::MenuItem("Spawn Zone"))
        {
            m_editor->CreateDefaultObject(MapObjectType::SPAWN_ZONE);
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // Object list
    int selectedIndex = m_editor->GetSelectedObjectIndex();

    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto &obj = objects[i];

        // Create display name
        std::string displayName = obj.name.empty() ? ("Object " + std::to_string(i)) : obj.name;

        // Add type icon/prefix
        const char *typeIcon = "[?] ";
        switch (obj.type)
        {
        case MapObjectType::CUBE:
            typeIcon = "[C] ";
            break;
        case MapObjectType::SPHERE:
            typeIcon = "[S] ";
            break;
        case MapObjectType::CYLINDER:
            typeIcon = "[Y] ";
            break;
        case MapObjectType::PLANE:
            typeIcon = "[P] ";
            break;
        case MapObjectType::LIGHT:
            typeIcon = "[L] ";
            break;
        case MapObjectType::MODEL:
            typeIcon = "[M] ";
            break;
        case MapObjectType::SPAWN_ZONE:
            typeIcon = "[Z] ";
            break;
        }

        bool isSelected = (static_cast<int>(i) == selectedIndex);

        if (ImGui::Selectable((typeIcon + displayName).c_str(), isSelected))
        {
            m_editor->SelectObject(static_cast<int>(i));
        }

        // Context menu
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                m_editor->RemoveObject(static_cast<int>(i));
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}
