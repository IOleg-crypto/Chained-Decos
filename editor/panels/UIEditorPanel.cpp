//
// UIEditorPanel.cpp - Implementation of UI Editor Panel
//

#include "UIEditorPanel.h"
#include "editor/IEditor.h"
#include "scene/ecs/components/UIComponents.h"
#include <imgui.h>
#include <raylib.h>

using namespace CHEngine;

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

    ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("UI Palette", &m_visible))
    {
        // 1. ADD WIDGETS (The main purpose now)
        if (ImGui::CollapsingHeader("Common Widgets", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderAddButtons();
            ImGui::Spacing();
        }

        // 2. SCENE SETTINGS (Keep for now, though could go to Inspector)
        if (ImGui::CollapsingHeader("Global UI Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderSceneSettings();
        }
    }
    ImGui::End();
}

void UIEditorPanel::RenderAddButtons()
{
    ImGui::Indent(10.0f);

    // ImGui Widgets (Main focus)
    ImGui::TextDisabled("ImGui Widgets (Recommended)");
    if (ImGui::Button("Button", ImVec2(80, 0)))
        AddImGuiButton();
    ImGui::SameLine();
    if (ImGui::Button("Text", ImVec2(80, 0)))
        AddImGuiText();
    ImGui::SameLine();
    if (ImGui::Button("Input", ImVec2(80, 0)))
        AddImGuiInput();
    ImGui::SameLine();
    if (ImGui::Button("Check", ImVec2(80, 0)))
        AddImGuiCheckbox();

    ImGui::Spacing();

    // Standard Widgets
    ImGui::TextDisabled("Standard Raylib Widgets");
    if (ImGui::Button("Panel##Ray", ImVec2(80, 0)))
        AddImage(); // Reusing Image as Panel for now
    ImGui::SameLine();
    if (ImGui::Button("Image##Ray", ImVec2(80, 0)))
        AddImage();

    ImGui::Unindent(10.0f);
}

void UIEditorPanel::RenderUIElementsList()
{
    auto &uiElements = m_editor->GetSceneManager().GetGameScene().GetUIElements();

    ImGui::Text("UI Elements (%d):", (int)uiElements.size());

    ImGui::BeginChild("UIElementsList", ImVec2(0, 150), true);

    for (int i = 0; i < uiElements.size(); i++)
    {
        const auto &elem = uiElements[i];
        bool isSelected = (m_editor->GetSelectionManager().GetSelectedUIElementIndex() == i);

        std::string label = elem.name + " (" + elem.type + ")";
        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            m_editor->GetSelectionManager().SelectUIElement(i);
        }
    }

    ImGui::EndChild();

    if (m_editor->GetSelectionManager().GetSelectedUIElementIndex() >= 0 &&
        ImGui::Button("Delete Selected"))
    {
        DeleteSelectedElement();
    }
}

void UIEditorPanel::RenderPropertiesPanel()
{
    auto &uiElements = m_editor->GetSceneManager().GetGameScene().GetUIElementsMutable();
    int selectedIndex = m_editor->GetSelectionManager().GetSelectedUIElementIndex();
    if (selectedIndex < 0 || selectedIndex >= uiElements.size())
        return;

    auto &elem = uiElements[selectedIndex];
    bool changed = false;

    // Use selectedIndex as ID scope to prevent conflicts with other elements or UI parts
    ImGui::PushID(selectedIndex);

    // 1. Header with Name and Active toggle
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    if (ImGui::Checkbox("##IsActive", &elem.isActive))
        changed = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle visibility/activity");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    char nameBuf[128];
    strncpy_s(nameBuf, elem.name.c_str(), sizeof(nameBuf) - 1);
    if (ImGui::InputText("##NameEdit", nameBuf, sizeof(nameBuf)))
    {
        elem.name = nameBuf;
        changed = true;
    }
    ImGui::PopStyleVar();

    ImGui::Separator();

    // 2. Component-based properties (Use unique IDs for headers)
    if (ImGui::CollapsingHeader("RectTransform##Header", ImGuiTreeNodeFlags_DefaultOpen))
    {
        RenderRectTransform(elem, changed);
    }

    if (elem.type == "button" || elem.type == "imgui_button")
    {
        if (ImGui::CollapsingHeader("Button Component##Header", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderButtonComponent(elem, changed);
        }
    }
    else if (elem.type == "text" || elem.type == "imgui_text")
    {
        if (ImGui::CollapsingHeader("Text Component##Header", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderTextComponent(elem, changed);
        }
    }
    else if (elem.type == "image")
    {
        if (ImGui::CollapsingHeader("Image Component##Header", ImGuiTreeNodeFlags_DefaultOpen))
        {
            RenderImageComponent(elem, changed);
        }
    }

    if (elem.type == "button" || elem.type == "imgui_button")
    {
        if (ImGui::CollapsingHeader("Actions##Header"))
        {
            RenderActionSystem(elem, changed);
        }
    }

    ImGui::PopID(); // selectedIndex

    if (changed)
    {
        m_editor->GetSceneManager().RefreshUIEntities();
        m_editor->GetSceneManager().SetSceneModified(true);
    }
}

void UIEditorPanel::DrawPropertyLabel(const char *label)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN);
}

void UIEditorPanel::RenderRectTransform(UIElementData &elem, bool &changed)
{
    if (ImGui::BeginTable("RectTransformTable", 2, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

        // Anchor
        const char *anchorNames[] = {"TopLeft",    "TopCenter",    "TopRight",
                                     "MiddleLeft", "MiddleCenter", "MiddleRight",
                                     "BottomLeft", "BottomCenter", "BottomRight"};
        DrawPropertyLabel("Anchor");
        if (ImGui::Combo("##Anchor", &elem.anchor, anchorNames, 9))
            changed = true;

        // Position
        DrawPropertyLabel("Position");
        if (ImGui::DragFloat2("##Position", &elem.position.x, 1.0f))
            changed = true;

        // Size
        DrawPropertyLabel("Size");
        if (ImGui::DragFloat2("##Size", &elem.size.x, 1.0f, 1.0f, 2048.0f))
            changed = true;

        // Pivot
        DrawPropertyLabel("Pivot");
        if (ImGui::SliderFloat2("##Pivot", &elem.pivot.x, 0.0f, 1.0f))
            changed = true;

        // Rotation
        DrawPropertyLabel("Rotation");
        if (ImGui::SliderFloat("##Rotation", &elem.rotation, 0.0f, 360.0f))
            changed = true;

        ImGui::EndTable();
    }
}

void UIEditorPanel::RenderTextComponent(UIElementData &elem, bool &changed)
{
    if (ImGui::BeginTable("TextTable", 2, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

        // Text Content
        DrawPropertyLabel("Text");
        char textBuf[256];
        strncpy_s(textBuf, elem.text.c_str(), sizeof(textBuf) - 1);
        if (ImGui::InputText("##Text", textBuf, sizeof(textBuf)))
        {
            elem.text = textBuf;
            changed = true;
        }

        // Font Size
        DrawPropertyLabel("Font Size");
        if (ImGui::DragInt("##FontSize", &elem.fontSize, 1.0f, 8, 128))
            changed = true;

        // Color
        DrawPropertyLabel("Color");
        float colT[4] = {elem.textColor.r / 255.0f, elem.textColor.g / 255.0f,
                         elem.textColor.b / 255.0f, elem.textColor.a / 255.0f};
        if (ImGui::ColorEdit4("##TextColor", colT))
        {
            elem.textColor = {(unsigned char)(colT[0] * 255), (unsigned char)(colT[1] * 255),
                              (unsigned char)(colT[2] * 255), (unsigned char)(colT[3] * 255)};
            changed = true;
        }

        // Font Name (placeholder for now)
        DrawPropertyLabel("Font");
        char fontBuf[128];
        strncpy_s(fontBuf, elem.fontName.c_str(), sizeof(fontBuf) - 1);
        if (ImGui::InputText("##FontName", fontBuf, sizeof(fontBuf)))
        {
            elem.fontName = fontBuf;
            changed = true;
        }

        DrawPropertyLabel("Spacing");
        if (ImGui::DragFloat("##Spacing", &elem.spacing, 0.1f, 0.0f, 10.0f))
            changed = true;

        ImGui::EndTable();
    }
}

void UIEditorPanel::RenderButtonComponent(UIElementData &elem, bool &changed)
{
    if (ImGui::BeginTable("ButtonTable", 2, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

        // Colors
        DrawPropertyLabel("Normal Color");
        float colN[4] = {elem.normalColor.r / 255.0f, elem.normalColor.g / 255.0f,
                         elem.normalColor.b / 255.0f, elem.normalColor.a / 255.0f};
        if (ImGui::ColorEdit4("##NormalColor", colN))
        {
            elem.normalColor = {(unsigned char)(colN[0] * 255), (unsigned char)(colN[1] * 255),
                                (unsigned char)(colN[2] * 255), (unsigned char)(colN[3] * 255)};
            changed = true;
        }

        DrawPropertyLabel("Hover Color");
        float colH[4] = {elem.hoverColor.r / 255.0f, elem.hoverColor.g / 255.0f,
                         elem.hoverColor.b / 255.0f, elem.hoverColor.a / 255.0f};
        if (ImGui::ColorEdit4("##HoverColor", colH))
        {
            elem.hoverColor = {(unsigned char)(colH[0] * 255), (unsigned char)(colH[1] * 255),
                               (unsigned char)(colH[2] * 255), (unsigned char)(colH[3] * 255)};
            changed = true;
        }

        // Border
        DrawPropertyLabel("Border Radius");
        if (ImGui::DragFloat("##BorderRadius", &elem.borderRadius, 0.5f, 0.0f, 50.0f))
            changed = true;

        DrawPropertyLabel("Border Width");
        if (ImGui::DragFloat("##BorderWidth", &elem.borderWidth, 0.1f, 0.0f, 10.0f))
            changed = true;

        DrawPropertyLabel("Border Color");
        float colB[4] = {elem.borderColor.r / 255.0f, elem.borderColor.g / 255.0f,
                         elem.borderColor.b / 255.0f, elem.borderColor.a / 255.0f};
        if (ImGui::ColorEdit4("##BorderColor", colB))
        {
            elem.borderColor = {(unsigned char)(colB[0] * 255), (unsigned char)(colB[1] * 255),
                                (unsigned char)(colB[2] * 255), (unsigned char)(colB[3] * 255)};
            changed = true;
        }

        ImGui::EndTable();
    }

    // Reuse Text rendering if it's a standard button
    if (elem.type == "button")
    {
        ImGui::Spacing();
        ImGui::TextDisabled("Button Content");
        RenderTextComponent(elem, changed);
    }
}

void UIEditorPanel::RenderImageComponent(UIElementData &elem, bool &changed)
{
    if (ImGui::BeginTable("ImageTable", 2, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

        DrawPropertyLabel("Tint");
        float colI[4] = {elem.tint.r / 255.0f, elem.tint.g / 255.0f, elem.tint.b / 255.0f,
                         elem.tint.a / 255.0f};
        if (ImGui::ColorEdit4("##Tint", colI))
        {
            elem.tint = {(unsigned char)(colI[0] * 255), (unsigned char)(colI[1] * 255),
                         (unsigned char)(colI[2] * 255), (unsigned char)(colI[3] * 255)};
            changed = true;
        }

        DrawPropertyLabel("Border Radius");
        if (ImGui::DragFloat("##BorderRadius", &elem.borderRadius, 0.5f, 0.0f, 50.0f))
            changed = true;

        DrawPropertyLabel("Border Width");
        if (ImGui::DragFloat("##BorderWidth", &elem.borderWidth, 0.1f, 0.0f, 10.0f))
            changed = true;

        ImGui::EndTable();
    }
}

void UIEditorPanel::RenderActionSystem(UIElementData &elem, bool &changed)
{
    if (ImGui::BeginTable("ActionTable", 2, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

        DrawPropertyLabel("Action Type");
        const char *actionTypes[] = {"None", "LoadScene", "Quit", "OpenURL"};
        int currentAction = 0;
        if (elem.actionType == "LoadScene")
            currentAction = 1;
        else if (elem.actionType == "Quit")
            currentAction = 2;
        else if (elem.actionType == "OpenURL")
            currentAction = 3;

        if (ImGui::Combo("##ActionType", &currentAction, actionTypes, IM_ARRAYSIZE(actionTypes)))
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

        if (elem.actionType == "LoadScene" || elem.actionType == "OpenURL")
        {
            DrawPropertyLabel(elem.actionType == "LoadScene" ? "Scene Path" : "URL");
            char targetBuf[256];
            strncpy_s(targetBuf, elem.actionTarget.c_str(), sizeof(targetBuf) - 1);
            if (ImGui::InputText("##ActionTarget", targetBuf, sizeof(targetBuf)))
            {
                elem.actionTarget = targetBuf;
                changed = true;
            }
        }

        DrawPropertyLabel("Event ID");
        char eventBuf[128];
        strncpy_s(eventBuf, elem.eventId.c_str(), sizeof(eventBuf) - 1);
        if (ImGui::InputText("##EventID", eventBuf, sizeof(eventBuf)))
        {
            elem.eventId = eventBuf;
            changed = true;
        }

        ImGui::EndTable();
    }
}

void UIEditorPanel::RenderSceneSettings()
{
    auto &metadata = m_editor->GetSceneManager().GetGameScene().GetMapMetaDataMutable();

    float bgColor[4] = {metadata.backgroundColor.r / 255.0f, metadata.backgroundColor.g / 255.0f,
                        metadata.backgroundColor.b / 255.0f, metadata.backgroundColor.a / 255.0f};

    if (ImGui::ColorEdit4("Background Color", bgColor))
    {
        metadata.backgroundColor = {
            (unsigned char)(bgColor[0] * 255), (unsigned char)(bgColor[1] * 255),
            (unsigned char)(bgColor[2] * 255), (unsigned char)(bgColor[3] * 255)};
        m_editor->GetSceneManager().SetSceneModified(true);
    }

    ImGui::Separator();
    ImGui::TextDisabled("Scene Settings");

    const char *sceneTypes[] = {"3D Map", "UI Menu", "Empty"};
    int currentType = static_cast<int>(metadata.sceneType);
    if (ImGui::Combo("Scene Type", &currentType, sceneTypes, IM_ARRAYSIZE(sceneTypes)))
    {
        metadata.sceneType = static_cast<SceneType>(currentType);
        m_editor->GetSceneManager().SetSceneModified(true);
    }

    ImGui::BulletText("Element Count: %d",
                      (int)m_editor->GetSceneManager().GetGameScene().GetUIElements().size());
}

void UIEditorPanel::AddButton()
{
    UIElementData elem;
    elem.type = "button";
    elem.name =
        "Button " +
        std::to_string(m_editor->GetSceneManager().GetGameScene().GetUIElements().size() + 1);
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
    elem.isActive = true;

    m_editor->GetSceneManager().GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->GetSelectionManager().SelectUIElement(
        (int)m_editor->GetSceneManager().GetGameScene().GetUIElements().size() - 1);

    m_editor->GetSceneManager().RefreshUIEntities();
    m_editor->GetSceneManager().SetSceneModified(true);
}

void UIEditorPanel::AddText()
{
    UIElementData elem;
    elem.type = "text";
    elem.name =
        "Text " +
        std::to_string(m_editor->GetSceneManager().GetGameScene().GetUIElements().size() + 1);
    elem.anchor = static_cast<int>(UIAnchor::TopLeft);
    elem.position = {m_newPosition[0], m_newPosition[1]};
    elem.size = {200.0f, 30.0f};
    elem.pivot = {0.0f, 0.0f};
    elem.rotation = 0.0f;
    elem.text = "Text Label";
    elem.fontSize = 20;
    elem.textColor = WHITE;
    elem.isActive = true;

    m_editor->GetSceneManager().GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->GetSelectionManager().SelectUIElement(
        (int)m_editor->GetSceneManager().GetGameScene().GetUIElements().size() - 1);

    m_editor->GetSceneManager().RefreshUIEntities();
    m_editor->GetSceneManager().SetSceneModified(true);
}

void UIEditorPanel::AddImage()
{
    UIElementData elem;
    elem.type = "image";
    elem.name =
        "Image " +
        std::to_string(m_editor->GetSceneManager().GetGameScene().GetUIElements().size() + 1);
    elem.anchor = static_cast<int>(UIAnchor::TopLeft);
    elem.position = {m_newPosition[0], m_newPosition[1]};
    elem.size = {100.0f, 100.0f};
    elem.pivot = {0.5f, 0.5f};
    elem.rotation = 0.0f;
    elem.tint = WHITE;
    elem.borderRadius = 0.0f;
    elem.borderWidth = 0.0f;
    elem.borderColor = BLACK;
    elem.isActive = true;

    m_editor->GetSceneManager().GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->GetSelectionManager().SelectUIElement(
        (int)m_editor->GetSceneManager().GetGameScene().GetUIElements().size() - 1);

    m_editor->GetSceneManager().RefreshUIEntities();
    m_editor->GetSceneManager().SetSceneModified(true);
}

void UIEditorPanel::AddImGuiButton()
{
    UIElementData elem;
    elem.type = "imgui_button";
    elem.name =
        "Button " +
        std::to_string(m_editor->GetSceneManager().GetGameScene().GetUIElements().size() + 1);
    elem.anchor = static_cast<int>(UIAnchor::TopLeft);
    elem.position = {100.0f, 100.0f};
    elem.size = {120.0f, 40.0f};
    elem.pivot = {0.5f, 0.5f};
    elem.rotation = 0.0f;
    elem.text = "Button";
    elem.fontSize = 16;
    elem.textColor = WHITE;
    elem.normalColor = GRAY;
    elem.hoverColor = LIGHTGRAY;
    elem.pressedColor = DARKGRAY;
    elem.borderRadius = 4.0f;
    elem.borderWidth = 1.0f;
    elem.borderColor = BLACK;
    elem.isActive = true;

    m_editor->GetSceneManager().GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->GetSelectionManager().SelectUIElement(
        (int)m_editor->GetSceneManager().GetGameScene().GetUIElements().size() - 1);

    m_editor->GetSceneManager().RefreshUIEntities();
    m_editor->GetSceneManager().SetSceneModified(true);
}

void UIEditorPanel::AddImGuiText()
{
    UIElementData elem;
    elem.type = "imgui_text";
    elem.name =
        "Text " +
        std::to_string(m_editor->GetSceneManager().GetGameScene().GetUIElements().size() + 1);
    elem.anchor = static_cast<int>(UIAnchor::TopLeft);
    elem.position = {100.0f, 150.0f};
    elem.size = {200.0f, 20.0f};
    elem.pivot = {0.0f, 0.0f};
    elem.rotation = 0.0f;
    elem.text = "Sample Text";
    elem.fontSize = 16;
    elem.textColor = WHITE;
    elem.isActive = true;

    m_editor->GetSceneManager().GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->GetSelectionManager().SelectUIElement(
        (int)m_editor->GetSceneManager().GetGameScene().GetUIElements().size() - 1);

    m_editor->GetSceneManager().RefreshUIEntities();
    m_editor->GetSceneManager().SetSceneModified(true);
}

void UIEditorPanel::AddImGuiInput()
{
    UIElementData elem;
    elem.type = "imgui_input";
    elem.name =
        "Input " +
        std::to_string(m_editor->GetSceneManager().GetGameScene().GetUIElements().size() + 1);
    elem.anchor = static_cast<int>(UIAnchor::TopLeft);
    elem.position = {100.0f, 200.0f};
    elem.size = {200.0f, 30.0f};
    elem.pivot = {0.0f, 0.0f};
    elem.rotation = 0.0f;
    elem.text = "";
    elem.fontSize = 16;
    elem.textColor = WHITE;
    elem.isActive = true;

    m_editor->GetSceneManager().GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->GetSelectionManager().SelectUIElement(
        (int)m_editor->GetSceneManager().GetGameScene().GetUIElements().size() - 1);

    m_editor->GetSceneManager().RefreshUIEntities();
    m_editor->GetSceneManager().SetSceneModified(true);
}

void UIEditorPanel::AddImGuiCheckbox()
{
    UIElementData elem;
    elem.type = "imgui_checkbox";
    elem.name =
        "Checkbox " +
        std::to_string(m_editor->GetSceneManager().GetGameScene().GetUIElements().size() + 1);
    elem.anchor = static_cast<int>(UIAnchor::TopLeft);
    elem.position = {100.0f, 250.0f};
    elem.size = {20.0f, 20.0f};
    elem.pivot = {0.0f, 0.0f};
    elem.rotation = 0.0f;
    elem.text = "Enable";
    elem.fontSize = 16;
    elem.textColor = WHITE;
    elem.isActive = true;

    m_editor->GetSceneManager().GetGameScene().GetUIElementsMutable().push_back(elem);
    m_editor->GetSelectionManager().SelectUIElement(
        (int)m_editor->GetSceneManager().GetGameScene().GetUIElements().size() - 1);

    m_editor->GetSceneManager().RefreshUIEntities();
    m_editor->GetSceneManager().SetSceneModified(true);
}

void UIEditorPanel::DeleteSelectedElement()
{
    auto &uiElements = m_editor->GetSceneManager().GetGameScene().GetUIElementsMutable();
    int selectedIndex = m_editor->GetSelectionManager().GetSelectedUIElementIndex();
    if (selectedIndex >= 0 && selectedIndex < (int)uiElements.size())
    {
        uiElements.erase(uiElements.begin() + selectedIndex);
        m_editor->GetSelectionManager().SelectUIElement(-1);

        m_editor->GetSceneManager().RefreshUIEntities();
        m_editor->GetSceneManager().SetSceneModified(true);
    }
}
