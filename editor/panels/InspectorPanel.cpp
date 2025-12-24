#include "InspectorPanel.h"
#include "nfd.h"
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <raylib.h>

namespace CHEngine
{
void InspectorPanel::OnImGuiRender(MapObjectData *selectedEntity)
{
    ImGui::Begin("Inspector");

    if (selectedEntity)
    {
        DrawComponents(selectedEntity);
    }
    else
    {
        ImGui::Text("No entity selected");
    }

    ImGui::End();
}

void InspectorPanel::DrawComponents(MapObjectData *entity)
{
    // 0. Name
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, entity->name.c_str(), sizeof(buffer));
    if (ImGui::InputText("##Name", buffer, sizeof(buffer)))
    {
        entity->name = std::string(buffer);
    }

    ImGui::Separator();

    // 1. Transform
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawVec3Control("Translation", entity->position);
        DrawVec3Control("Rotation", entity->rotation);
        DrawVec3Control("Scale", entity->scale, 1.0f);
    }

    // 2. Properties (Type specific)
    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const char *typeNames[] = {"Cube",  "Sphere", "Cylinder",  "Plane",
                                   "Light", "Model",  "Spawn Zone"};
        ImGui::Text("Type: %s", typeNames[(int)entity->type]);

        if (entity->type == MapObjectType::SPHERE)
        {
            ImGui::DragFloat("Radius", &entity->radius, 0.1f);
        }
        else if (entity->type == MapObjectType::CYLINDER)
        {
            ImGui::DragFloat("Height", &entity->height, 0.1f);
        }
        else if (entity->type == MapObjectType::PLANE)
        {
            ImGui::DragFloat2("Size", &entity->size.x, 0.1f);
        }

        float color[4] = {entity->color.r / 255.0f, entity->color.g / 255.0f,
                          entity->color.b / 255.0f, entity->color.a / 255.0f};
        if (ImGui::ColorEdit4("Color", color))
        {
            entity->color.r = (unsigned char)(color[0] * 255.0f);
            entity->color.g = (unsigned char)(color[1] * 255.0f);
            entity->color.b = (unsigned char)(color[2] * 255.0f);
            entity->color.a = (unsigned char)(color[3] * 255.0f);
        }

        ImGui::Checkbox("Is Platform", &entity->isPlatform);
        ImGui::Checkbox("Is Obstacle", &entity->isObstacle);
    }

    // 3. Scripting
    if (ImGui::CollapsingHeader("Scripting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        char scriptBuffer[256];
        memset(scriptBuffer, 0, sizeof(scriptBuffer));
        strncpy(scriptBuffer, entity->scriptPath.c_str(), sizeof(scriptBuffer));

        if (ImGui::InputText("Script Path", scriptBuffer, sizeof(scriptBuffer)))
        {
            entity->scriptPath = std::string(scriptBuffer);
        }

        ImGui::SameLine();
        if (ImGui::Button("..."))
        {
            nfdfilteritem_t filterItem[1] = {{"Lua Script", "lua"}};
            nfdchar_t *outPath = nullptr;
            nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

            if (result == NFD_OKAY)
            {
                entity->scriptPath = outPath;
                NFD_FreePath(outPath);
            }
        }
    }
}

void InspectorPanel::OnImGuiRender(UIElementData *selectedElement)
{
    ImGui::Begin("Inspector");

    if (selectedElement)
    {
        DrawUIComponents(selectedElement);
    }
    else
    {
        ImGui::Text("No element selected");
    }

    ImGui::End();
}

void InspectorPanel::DrawUIComponents(UIElementData *element)
{
    // Name
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, element->name.c_str(), sizeof(buffer));
    if (ImGui::InputText("##Name", buffer, sizeof(buffer)))
    {
        element->name = std::string(buffer);
    }

    ImGui::Separator();

    // Transform (RectTransform)
    if (ImGui::CollapsingHeader("Rect Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float pos[2] = {element->position.x, element->position.y};
        if (ImGui::DragFloat2("Position", pos))
        {
            element->position = {pos[0], pos[1]};
        }

        float size[2] = {element->size.x, element->size.y};
        if (ImGui::DragFloat2("Size", size))
        {
            element->size = {size[0], size[1]};
        }

        ImGui::DragInt("Anchor", &element->anchor, 0.1f, 0, 8);
    }

    // Type Specific
    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (element->type == "text" || element->type == "button")
        {
            char textBuffer[256];
            memset(textBuffer, 0, sizeof(textBuffer));
            strncpy(textBuffer, element->text.c_str(), sizeof(textBuffer));
            if (ImGui::InputTextMultiline("Text", textBuffer, sizeof(textBuffer)))
            {
                element->text = std::string(textBuffer);
            }

            ImGui::DragInt("Font Size", &element->fontSize, 1, 1, 100);

            float color[4] = {element->textColor.r / 255.0f, element->textColor.g / 255.0f,
                              element->textColor.b / 255.0f, element->textColor.a / 255.0f};
            if (ImGui::ColorEdit4("Text Color", color))
            {
                element->textColor.r = (unsigned char)(color[0] * 255.0f);
                element->textColor.g = (unsigned char)(color[1] * 255.0f);
                element->textColor.b = (unsigned char)(color[2] * 255.0f);
                element->textColor.a = (unsigned char)(color[3] * 255.0f);
            }
        }

        if (element->type == "button")
        {
            ImGui::Text("Button Colors");
            // Normal
            float nColor[4] = {element->normalColor.r / 255.0f, element->normalColor.g / 255.0f,
                               element->normalColor.b / 255.0f, element->normalColor.a / 255.0f};
            if (ImGui::ColorEdit4("Normal", nColor))
            {
                element->normalColor.r = (unsigned char)(nColor[0] * 255.0f);
                element->normalColor.g = (unsigned char)(nColor[1] * 255.0f);
                element->normalColor.b = (unsigned char)(nColor[2] * 255.0f);
                element->normalColor.a = (unsigned char)(nColor[3] * 255.0f);
            }
            // Hover
            float hColor[4] = {element->hoverColor.r / 255.0f, element->hoverColor.g / 255.0f,
                               element->hoverColor.b / 255.0f, element->hoverColor.a / 255.0f};
            if (ImGui::ColorEdit4("Hover", hColor))
            {
                element->hoverColor.r = (unsigned char)(hColor[0] * 255.0f);
                element->hoverColor.g = (unsigned char)(hColor[1] * 255.0f);
                element->hoverColor.b = (unsigned char)(hColor[2] * 255.0f);
                element->hoverColor.a = (unsigned char)(hColor[3] * 255.0f);
            }
            // Pressed
            float pColor[4] = {element->pressedColor.r / 255.0f, element->pressedColor.g / 255.0f,
                               element->pressedColor.b / 255.0f, element->pressedColor.a / 255.0f};
            if (ImGui::ColorEdit4("Pressed", pColor))
            {
                element->pressedColor.r = (unsigned char)(pColor[0] * 255.0f);
                element->pressedColor.g = (unsigned char)(pColor[1] * 255.0f);
                element->pressedColor.b = (unsigned char)(pColor[2] * 255.0f);
                element->pressedColor.a = (unsigned char)(pColor[3] * 255.0f);
            }
        }
    }
}

void InspectorPanel::DrawVec3Control(const std::string &label, Vector3 &values, float resetValue,
                                     float columnWidth)
{
    ImGuiIO &io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0]; // For now use default, in real app would be bold

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

    float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    if (ImGui::Button("X", buttonSize))
        values.x = resetValue;
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    if (ImGui::Button("Y", buttonSize))
        values.y = resetValue;
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    if (ImGui::Button("Z", buttonSize))
        values.z = resetValue;
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();
}
} // namespace CHEngine
