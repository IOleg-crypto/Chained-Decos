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

    // Get objects from game scene
    auto &gameScene = m_editor->GetSceneManager().GetGameScene();
    auto &objects = gameScene.GetMapObjectsMutable();

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
            m_editor->GetSceneManager().CreateDefaultObject(MapObjectType::CUBE);
        }
        if (ImGui::MenuItem("Sphere"))
        {
            m_editor->GetSceneManager().CreateDefaultObject(MapObjectType::SPHERE);
        }
        if (ImGui::MenuItem("Cylinder"))
        {
            m_editor->GetSceneManager().CreateDefaultObject(MapObjectType::CYLINDER);
        }
        if (ImGui::MenuItem("Plane"))
        {
            m_editor->GetSceneManager().CreateDefaultObject(MapObjectType::PLANE);
        }
        if (ImGui::MenuItem("Model"))
        {
            m_editor->GetSceneManager().CreateDefaultObject(MapObjectType::MODEL);
        }
        if (ImGui::MenuItem("Spawn Zone"))
        {
            m_editor->GetSceneManager().CreateDefaultObject(MapObjectType::SPAWN_ZONE);
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // Object list
    int selectedIndex = m_editor->GetSelectionManager().GetSelectedObjectIndex();

    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto &obj = objects[i];
        bool isSelected = (static_cast<int>(i) == selectedIndex);

        ImGui::PushID(static_cast<int>(i));

        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf |
                                       ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                       ImGuiTreeNodeFlags_SpanAvailWidth;
        if (isSelected)
            nodeFlags |= ImGuiTreeNodeFlags_Selected;

        // Icon and name based on type
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
        case MapObjectType::MODEL:
            typeIcon = "[M] ";
            break;
        case MapObjectType::SPAWN_ZONE:
            typeIcon = "[Z] ";
            break;
        default:
            break;
        }

        std::string label = std::string(typeIcon) + (obj.name.empty() ? "Object" : obj.name);

        if (ImGui::TreeNodeEx(label.c_str(), nodeFlags))
        {
            if (ImGui::IsItemClicked())
            {
                m_editor->GetSelectionManager().SelectObject(static_cast<int>(i));
            }
        }

        // Context menu
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                m_editor->GetSceneManager().RemoveObject(static_cast<int>(i));
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::Text("UI Elements (%zu):", gameScene.GetUIElements().size());

    int selectedUIIndex = m_editor->GetSelectionManager().GetSelectedUIElementIndex();
    const auto &uiElements = gameScene.GetUIElements();

    for (size_t i = 0; i < uiElements.size(); ++i)
    {
        const auto &ui = uiElements[i];
        std::string displayName = ui.name.empty() ? ("UI Element " + std::to_string(i)) : ui.name;

        std::string typeIcon = "[UI] ";
        if (ui.type == "button")
            typeIcon = "[Btn] ";
        else if (ui.type == "text")
            typeIcon = "[Txt] ";
        else if (ui.type == "image")
            typeIcon = "[Img] ";

        bool isSelected = (static_cast<int>(i) == selectedUIIndex);

        if (ImGui::Selectable((typeIcon + displayName).c_str(), isSelected))
        {
            m_editor->GetSelectionManager().SelectUIElement(static_cast<int>(i));
            // Deselect 3D object when selecting UI
            m_editor->GetSelectionManager().SelectObject(-1);
        }

        if (isSelected && m_editor->GetState().IsUIDesignMode())
        {
            // Visual hint in hierarchy that we're in UI mode
            ImGui::SameLine();
            ImGui::TextDisabled("(Active)");
        }
    }

    ImGui::End();
}
