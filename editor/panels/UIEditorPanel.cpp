//
// UIEditorPanel.cpp - Implementation of UI Editor Panel
//

#include "UIEditorPanel.h"
#include "scene/ecs/components/UIComponents.h"
#include "editor/IEditor.h"
#include <imgui.h>
#include <raylib.h>

using namespace ChainedDecos;

UIEditorPanel::UIEditorPanel(IEditor *editor) : m_editor(editor)
{
}

void UIEditorPanel::Update(float deltaTime)
{
    // Update logic if needed
}

const char *UIEditorPanel::GetName() const
{
    return "UIEditor";
}

const char *UIEditorPanel::GetDisplayName() const
{
    return "UI Editor";
}

bool UIEditorPanel::IsVisible() const
{
    return m_visible;
}

void UIEditorPanel::SetVisible(bool visible)
{
    m_visible = visible;
}

void UIEditorPanel::Render()
{
    if (!m_visible)
        return;

    ImGui::Begin("UI Editor", &m_visible);

    // Add buttons section
    RenderAddButtons();

    ImGui::Separator();

    // UI Elements list
    RenderUIElementsList();

    ImGui::Separator();

    // Properties panel for selected element
    // Access elements from GameScene
    auto &uiElements = m_editor->GetGameScene().GetUIElementsMutable();

    int selectedIdx = m_editor->GetSelectedUIElementIndex();
    if (selectedIdx >= 0 && selectedIdx < uiElements.size())
    {
        RenderPropertiesPanel();
    }
    else if (selectedIdx != -1)
    {
        m_editor->SelectUIElement(-1);
    }

    ImGui::End();
}

void UIEditorPanel::RenderAddButtons()
{
    ImGui::Text("Add UI Element:");

    if (ImGui::Button("Add Button", ImVec2(100, 0)))
    {
        AddButton();
    }
    ImGui::SameLine();

    if (ImGui::Button("Add Text", ImVec2(100, 0)))
    {
        AddText();
    }
    ImGui::SameLine();

    if (ImGui::Button("Add Image", ImVec2(100, 0)))
    {
        AddImage();
    }
    ImGui::SameLine();

    if (ImGui::Button("Add ImGui Button", ImVec2(120, 0)))
    {
        AddImGuiButton();
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh View"))
    {
        m_editor->RefreshUIEntities();
    }
}

void UIEditorPanel::RenderUIElementsList()
{
    auto &uiElements = m_editor->GetGameScene().GetUIElements();

    ImGui::Text("UI Elements (%d):", (int)uiElements.size());

    ImGui::BeginChild("UIElementsList", ImVec2(0, 150), true);

    for (int i = 0; i < uiElements.size(); i++)
    {
        const auto &elem = uiElements[i];
        bool isSelected = (m_editor->GetSelectedUIElementIndex() == i);

        std::string label = elem.name + " (" + elem.type + ")";
        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            m_editor->SelectUIElement(i);
        }
    }

    ImGui::EndChild();

    if (m_editor->GetSelectedUIElementIndex() >= 0 && ImGui::Button("Delete Selected"))
    {
        DeleteSelectedElement();
    }
}

void UIEditorPanel::RenderPropertiesPanel()
{
    auto &uiElements = m_editor->GetGameScene().GetUIElementsMutable();
    int selectedIndex = m_editor->GetSelectedUIElementIndex();
    if (selectedIndex < 0 || selectedIndex >= uiElements.size())
        return;

    auto &elem = uiElements[selectedIndex];

    bool changed = false;

    ImGui::Text("Properties:");

    // Type (read-only)
    ImGui::Text("Type: %s", elem.type.c_str());

    // Anchor
    const char *anchorNames[] = {"TopLeft",    "TopCenter",    "TopRight",
                                 "MiddleLeft", "MiddleCenter", "MiddleRight",
                                 "BottomLeft", "BottomCenter", "BottomRight"};
    if (ImGui::Combo("Anchor", &elem.anchor, anchorNames, 9))
        changed = true;

    // Position
    if (ImGui::DragFloat2("Position", &elem.position.x, 1.0f))
        changed = true;

    // Size
    if (ImGui::DragFloat2("Size", &elem.size.x, 1.0f, 10.0f, 1000.0f))
        changed = true;

    // Pivot
    if (ImGui::SliderFloat2("Pivot", &elem.pivot.x, 0.0f, 1.0f))
        changed = true;

    // Rotation
    if (ImGui::SliderFloat("Rotation", &elem.rotation, 0.0f, 360.0f))
        changed = true;

    ImGui::Separator();

    // Type-specific properties
    if (elem.type == "button")
    {
        char textBuf[256];
        strncpy_s(textBuf, elem.text.c_str(), sizeof(textBuf) - 1);
        if (ImGui::InputText("Text", textBuf, sizeof(textBuf)))
        {
            elem.text = textBuf;
            changed = true;
        }

        if (ImGui::DragInt("Font Size", &elem.fontSize, 1.0f, 8, 72))
            changed = true;

        char fontBuf[128];
        strncpy_s(fontBuf, elem.fontName.c_str(), sizeof(fontBuf) - 1);
        if (ImGui::InputText("Font Name", fontBuf, sizeof(fontBuf)))
        {
            elem.fontName = fontBuf;
            changed = true;
        }

        if (ImGui::DragFloat("Spacing", &elem.spacing, 0.1f, 0.0f, 10.0f))
            changed = true;

        float colN[4] = {elem.normalColor.r / 255.0f, elem.normalColor.g / 255.0f,
                         elem.normalColor.b / 255.0f, elem.normalColor.a / 255.0f};
        if (ImGui::ColorEdit4("Normal Color", colN))
        {
            elem.normalColor = {(unsigned char)(colN[0] * 255), (unsigned char)(colN[1] * 255),
                                (unsigned char)(colN[2] * 255), (unsigned char)(colN[3] * 255)};
            changed = true;
        }

        float colH[4] = {elem.hoverColor.r / 255.0f, elem.hoverColor.g / 255.0f,
                         elem.hoverColor.b / 255.0f, elem.hoverColor.a / 255.0f};
        if (ImGui::ColorEdit4("Hover Color", colH))
        {
            elem.hoverColor = {(unsigned char)(colH[0] * 255), (unsigned char)(colH[1] * 255),
                               (unsigned char)(colH[2] * 255), (unsigned char)(colH[3] * 255)};
            changed = true;
        }

        if (ImGui::DragFloat("Border Radius", &elem.borderRadius, 1.0f, 0.0f, 100.0f))
            changed = true;

        if (ImGui::DragFloat("Border Width", &elem.borderWidth, 0.5f, 0.0f, 10.0f))
            changed = true;

        float colB[4] = {elem.borderColor.r / 255.0f, elem.borderColor.g / 255.0f,
                         elem.borderColor.b / 255.0f, elem.borderColor.a / 255.0f};
        if (ImGui::ColorEdit4("Border Color", colB))
        {
            elem.borderColor = {(unsigned char)(colB[0] * 255), (unsigned char)(colB[1] * 255),
                                (unsigned char)(colB[2] * 255), (unsigned char)(colB[3] * 255)};
            changed = true;
        }

        char eventBuf[128];
        strncpy_s(eventBuf, elem.eventId.c_str(), sizeof(eventBuf) - 1);
        if (ImGui::InputText("Event ID", eventBuf, sizeof(eventBuf)))
        {
            elem.eventId = eventBuf;
            changed = true;
        }

        ImGui::Separator();
        ImGui::Text("On Click Action:");

        // Action Type
        const char *actionTypes[] = {"None", "LoadScene", "Quit", "OpenURL"};
        int currentAction = 0;
        if (elem.actionType == "LoadScene")
            currentAction = 1;
        else if (elem.actionType == "Quit")
            currentAction = 2;
        else if (elem.actionType == "OpenURL")
            currentAction = 3;

        if (ImGui::Combo("Action Type", &currentAction, actionTypes, IM_ARRAYSIZE(actionTypes)))
        {
            if (currentAction == 0)
                elem.actionType = "None";
            else if (currentAction == 1)
                elem.actionType = "LoadScene";
            else if (currentAction == 2)
                elem.actionType = "Quit";
            else if (currentAction == 3)
                elem.actionType = "OpenURL";
            changed = true;
        }

        // Action Target (only if needed)
        if (elem.actionType == "LoadScene" || elem.actionType == "OpenURL")
        {
            char targetBuf[256];
            strncpy_s(targetBuf, elem.actionTarget.c_str(), sizeof(targetBuf) - 1);
            if (ImGui::InputText(elem.actionType == "LoadScene" ? "Scene Path" : "URL", targetBuf,
                                 sizeof(targetBuf)))
            {
                elem.actionTarget = targetBuf;
                changed = true;
            }
        }
    }
    else if (elem.type == "text")
    {
        char textBuf[256];
        strncpy_s(textBuf, elem.text.c_str(), sizeof(textBuf) - 1);
        if (ImGui::InputText("Text", textBuf, sizeof(textBuf)))
        {
            elem.text = textBuf;
            changed = true;
        }

        if (ImGui::DragInt("Font Size", &elem.fontSize, 1.0f, 8, 72))
            changed = true;

        char fontBuf[128];
        strncpy_s(fontBuf, elem.fontName.c_str(), sizeof(fontBuf) - 1);
        if (ImGui::InputText("Font Name", fontBuf, sizeof(fontBuf)))
        {
            elem.fontName = fontBuf;
            changed = true;
        }

        if (ImGui::DragFloat("Spacing", &elem.spacing, 0.1f, 0.0f, 10.0f))
            changed = true;
        float colT[4] = {elem.textColor.r / 255.0f, elem.textColor.g / 255.0f,
                         elem.textColor.b / 255.0f, elem.textColor.a / 255.0f};
        if (ImGui::ColorEdit4("Text Color", colT))
        {
            elem.textColor = {(unsigned char)(colT[0] * 255), (unsigned char)(colT[1] * 255),
                              (unsigned char)(colT[2] * 255), (unsigned char)(colT[3] * 255)};
            changed = true;
        }
    }
    else if (elem.type == "image")
    {
        float colI[4] = {elem.tint.r / 255.0f, elem.tint.g / 255.0f, elem.tint.b / 255.0f,
                         elem.tint.a / 255.0f};
        if (ImGui::ColorEdit4("Tint", colI))
        {
            elem.tint = {(unsigned char)(colI[0] * 255), (unsigned char)(colI[1] * 255),
                         (unsigned char)(colI[2] * 255), (unsigned char)(colI[3] * 255)};
            changed = true;
        }

        if (ImGui::DragFloat("Border Radius", &elem.borderRadius, 1.0f, 0.0f, 100.0f))
            changed = true;

        if (ImGui::DragFloat("Border Width", &elem.borderWidth, 0.5f, 0.0f, 10.0f))
            changed = true;

        float colB[4] = {elem.borderColor.r / 255.0f, elem.borderColor.g / 255.0f,
                         elem.borderColor.b / 255.0f, elem.borderColor.a / 255.0f};
        if (ImGui::ColorEdit4("Border Color", colB))
        {
            elem.borderColor = {(unsigned char)(colB[0] * 255), (unsigned char)(colB[1] * 255),
                                (unsigned char)(colB[2] * 255), (unsigned char)(colB[3] * 255)};
            changed = true;
        }
    }
    else if (elem.type == "imgui_button")
    {
        ImGui::Text("ImGui Button Properties");
        // Label is sync'd with name for now
        ImGui::Text("Label: %s", elem.name.c_str());
    }

    if (changed)
    {
        m_editor->RefreshUIEntities();
        m_editor->SetSceneModified(true);
    }
}

void UIEditorPanel::AddButton()
{
    UIElementData elem;
    elem.type = "button";
    elem.name = "Button " + std::to_string(m_editor->GetGameScene().GetUIElements().size() + 1);
    elem.anchor = static_cast<int>(UIAnchor::TopLeft);
    elem.position = {m_newPosition[0], m_newPosition[1]};
    elem.size = {m_newSize[0], m_newSize[1]};
    elem.pivot = {0.5f, 0.5f};
    elem.rotation = 0.0f;
    elem.text = m_newText;
    elem.fontSize = m_newFontSize;
    elem.textColor = WHITE;
    elem.normalColor = GRAY;
    elem.hoverColor = LIGHTGRAY;
    elem.pressedColor = DARKGRAY;
    elem.eventId = "button_click";

    m_editor->GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->SelectUIElement((int)m_editor->GetGameScene().GetUIElements().size() - 1);

    m_editor->RefreshUIEntities();
    m_editor->SetSceneModified(true);
}

void UIEditorPanel::AddText()
{
    UIElementData elem;
    elem.type = "text";
    elem.name = "Text " + std::to_string(m_editor->GetGameScene().GetUIElements().size() + 1);
    elem.anchor = static_cast<int>(UIAnchor::TopLeft);
    elem.position = {m_newPosition[0], m_newPosition[1]};
    elem.size = {200.0f, 30.0f};
    elem.pivot = {0.0f, 0.0f};
    elem.rotation = 0.0f;
    elem.text = "Text Label";
    elem.fontSize = 20;
    elem.textColor = WHITE;

    m_editor->GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->SelectUIElement((int)m_editor->GetGameScene().GetUIElements().size() - 1);

    m_editor->RefreshUIEntities();
    m_editor->SetSceneModified(true);
}

void UIEditorPanel::AddImage()
{
    UIElementData elem;
    elem.type = "image";
    elem.name = "Image " + std::to_string(m_editor->GetGameScene().GetUIElements().size() + 1);
    elem.anchor = static_cast<int>(UIAnchor::TopLeft);
    elem.position = {m_newPosition[0], m_newPosition[1]};
    elem.size = {100.0f, 100.0f};
    elem.pivot = {0.5f, 0.5f};
    elem.rotation = 0.0f;
    elem.tint = WHITE;

    m_editor->GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->SelectUIElement((int)m_editor->GetGameScene().GetUIElements().size() - 1);

    m_editor->RefreshUIEntities();
    m_editor->SetSceneModified(true);
}

void UIEditorPanel::AddImGuiButton()
{
    UIElementData elem;
    elem.type = "imgui_button";
    elem.name = "ImBtn " + std::to_string(m_editor->GetGameScene().GetUIElements().size() + 1);
    elem.anchor = static_cast<int>(UIAnchor::TopLeft);
    elem.position = {m_newPosition[0], m_newPosition[1]};
    elem.size = {120.0f, 40.0f};
    elem.pivot = {0.5f, 0.5f};
    elem.rotation = 0.0f;

    m_editor->GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->SelectUIElement((int)m_editor->GetGameScene().GetUIElements().size() - 1);

    m_editor->RefreshUIEntities();
    m_editor->SetSceneModified(true);
}

void UIEditorPanel::DeleteSelectedElement()
{
    auto &uiElements = m_editor->GetGameScene().GetUIElementsMutable();
    int selectedIndex = m_editor->GetSelectedUIElementIndex();
    if (selectedIndex >= 0 && selectedIndex < (int)uiElements.size())
    {
        uiElements.erase(uiElements.begin() + selectedIndex);
        m_editor->SelectUIElement(-1);

        m_editor->RefreshUIEntities();
        m_editor->SetSceneModified(true);
    }
}
