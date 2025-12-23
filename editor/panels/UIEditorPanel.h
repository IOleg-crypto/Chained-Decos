//
// UIEditorPanel.h - Panel for creating and editing UI elements
//

#ifndef UI_EDITOR_PANEL_H
#define UI_EDITOR_PANEL_H

#include "IEditorPanel.h"
#include "scene/resources/map/core/MapData.h"

class IEditor;

class UIEditorPanel : public IEditorPanel
{
public:
    explicit UIEditorPanel(IEditor *editor);
    ~UIEditorPanel() override = default;

    // IEditorPanel interface
    void Render() override;
    void Update(float deltaTime) override;
    const char *GetName() const override;
    const char *GetDisplayName() const override;
    bool IsVisible() const override;
    void SetVisible(bool visible) override;

private:
    void RenderUIElementsList();
    void RenderAddButtons();
    void RenderPropertiesPanel();
    void RenderSceneSettings();

    // Inspector Helpers (Unity-style decomposition)
    void DrawPropertyLabel(const char *label);
    void RenderRectTransform(UIElementData &elem, bool &changed);
    void RenderTextComponent(UIElementData &elem, bool &changed);
    void RenderButtonComponent(UIElementData &elem, bool &changed);
    void RenderImageComponent(UIElementData &elem, bool &changed);
    void RenderActionSystem(UIElementData &elem, bool &changed);

    void AddButton();
    void AddText();
    void AddImage();
    void AddImGuiButton();
    void AddImGuiText();
    void AddImGuiInput();
    void AddImGuiCheckbox();
    void DeleteSelectedElement();

    void CreateEntityFromUIElement(const UIElementData &elemData);
    void RefreshUIEntities();

    IEditor *m_editor;
    bool m_visible = true;

    // Temporary data for new element creation
    char m_newElementName[128] = "New Element";
    char m_newText[256] = "Button";
    int m_newFontSize = 20;
    float m_newPosition[2] = {100.0f, 100.0f};
    float m_newSize[2] = {120.0f, 40.0f};
    int m_newAnchor = 0; // UIAnchor::TopLeft
};

#endif // UI_EDITOR_PANEL_H
