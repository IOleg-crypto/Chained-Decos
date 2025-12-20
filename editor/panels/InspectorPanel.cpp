//
// InspectorPanel.cpp - Implementation of object inspector panel
//

#include "InspectorPanel.h"
#include "editor/IEditor.h"
#include "editor/mapgui/IUIManager.h"
#include <cstring>
#include <imgui.h>

InspectorPanel::InspectorPanel(IEditor *editor) : m_editor(editor)
{
}

void InspectorPanel::Render()
{
    if (!m_visible)
        return;

    ImGui::Begin("Inspector", &m_visible);

    if (!m_editor)
    {
        ImGui::Text("No editor instance");
        ImGui::End();
        return;
    }

    MapObjectData *selected = m_editor->GetSelectedObject();
    if (!selected)
    {
        ImGui::Text("Global Settings");
        ImGui::Separator();

        // Skybox preview/button
        std::string skyboxName = m_editor->GetGameMap().GetMapMetaData().skyboxTexture;
        if (skyboxName.empty())
            skyboxName = "None";
        ImGui::Text("Active Skybox: %s", skyboxName.c_str());

        if (ImGui::Button("Change Skybox..."))
        {
            if (auto ui = m_editor->GetUIManager())
            {
                ui->ToggleSkyboxBrowser();
            }
        }

        ImGui::Separator();

        // Clear Color
        Color clearColor = m_editor->GetClearColor();
        float col[4] = {clearColor.r / 255.0f, clearColor.g / 255.0f, clearColor.b / 255.0f,
                        clearColor.a / 255.0f};
        if (ImGui::ColorEdit4("Clear Color", col))
        {
            Color newCol = {
                static_cast<unsigned char>(col[0] * 255), static_cast<unsigned char>(col[1] * 255),
                static_cast<unsigned char>(col[2] * 255), static_cast<unsigned char>(col[3] * 255)};
            // Note: need an API to set this permanently in metadata
            auto metadata = m_editor->GetGameMap().GetMapMetaData();
            metadata.skyColor = newCol;
            m_editor->ApplyMetadata(metadata);
            m_editor->SetSceneModified(true);
        }

        ImGui::End();
        return;
    }

    // Object name
    char nameBuffer[256];
    strncpy(nameBuffer, selected->name.c_str(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';

    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
    {
        selected->name = nameBuffer;
        m_editor->SetSceneModified(true);
    }

    ImGui::Separator();

    // Transform section
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        RenderTransform(selected);
    }

    // Object-specific properties
    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        RenderObjectProperties(selected);
    }

    ImGui::End();
}

void InspectorPanel::RenderTransform(MapObjectData *obj)
{
    bool modified = false;

    // Position
    float pos[3] = {obj->position.x, obj->position.y, obj->position.z};
    if (ImGui::DragFloat3("Position", pos, 0.1f))
    {
        obj->position = {pos[0], pos[1], pos[2]};
        modified = true;
    }

    // Rotation
    float rot[3] = {obj->rotation.x, obj->rotation.y, obj->rotation.z};
    if (ImGui::DragFloat3("Rotation", rot, 1.0f))
    {
        obj->rotation = {rot[0], rot[1], rot[2]};
        modified = true;
    }

    // Scale
    float scale[3] = {obj->scale.x, obj->scale.y, obj->scale.z};
    if (ImGui::DragFloat3("Scale", scale, 0.05f, 0.01f, 100.0f))
    {
        obj->scale = {scale[0], scale[1], scale[2]};
        modified = true;
    }

    if (modified)
    {
        m_editor->SetSceneModified(true);
    }
}

void InspectorPanel::RenderObjectProperties(MapObjectData *obj)
{
    // Object type (read-only)
    const char *typeNames[] = {"Cube",  "Sphere", "Cylinder",  "Plane",
                               "Light", "Model",  "Spawn Zone"};
    int typeIndex = static_cast<int>(obj->type);
    if (typeIndex >= 0 && typeIndex < 7)
    {
        ImGui::Text("Type: %s", typeNames[typeIndex]);
    }

    // Model name (for MODEL type)
    if (obj->type == MapObjectType::MODEL && !obj->modelName.empty())
    {
        ImGui::Text("Model: %s", obj->modelName.c_str());
    }

    // Sphere radius
    if (obj->type == MapObjectType::SPHERE)
    {
        if (ImGui::DragFloat("Radius", &obj->radius, 0.1f, 0.1f, 100.0f))
        {
            m_editor->SetSceneModified(true);
        }
    }

    // Cylinder height
    if (obj->type == MapObjectType::CYLINDER)
    {
        if (ImGui::DragFloat("Height", &obj->height, 0.1f, 0.1f, 100.0f))
        {
            m_editor->SetSceneModified(true);
        }
    }

    // Platform/Obstacle checkboxes
    if (ImGui::Checkbox("Is Platform", &obj->isPlatform))
    {
        m_editor->SetSceneModified(true);
    }
    if (ImGui::Checkbox("Is Obstacle", &obj->isObstacle))
    {
        m_editor->SetSceneModified(true);
    }

    // Color picker
    float color[4] = {obj->color.r / 255.0f, obj->color.g / 255.0f, obj->color.b / 255.0f,
                      obj->color.a / 255.0f};
    if (ImGui::ColorEdit4("Color", color))
    {
        obj->color.r = static_cast<unsigned char>(color[0] * 255);
        obj->color.g = static_cast<unsigned char>(color[1] * 255);
        obj->color.b = static_cast<unsigned char>(color[2] * 255);
        obj->color.a = static_cast<unsigned char>(color[3] * 255);
        m_editor->SetSceneModified(true);
    }
}
