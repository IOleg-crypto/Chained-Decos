//
// ViewportPanel.h - 3D viewport panel for scene rendering
//

#ifndef VIEWPORTPANEL_H
#define VIEWPORTPANEL_H

#include "IEditorPanel.h"
#include <imgui.h>
#include <raylib.h>

class IEditor;

// Displays the 3D scene viewport with render texture
class ViewportPanel : public IEditorPanel
{
public:
    explicit ViewportPanel(IEditor *editor);
    ~ViewportPanel() override;

    // IEditorPanel interface
    void Render() override;
    void Update(float deltaTime) override;
    const char *GetName() const override
    {
        return "Viewport";
    }
    const char *GetDisplayName() const override
    {
        return "Scene Viewport";
    }
    bool IsVisible() const override
    {
        return m_visible;
    }
    void SetVisible(bool visible) override
    {
        m_visible = visible;
    }

    // Viewport-specific
    bool IsHovered() const
    {
        return m_isHovered;
    }
    bool IsFocused() const
    {
        return m_isFocused;
    }
    ImVec2 GetViewportSize() const
    {
        return m_viewportSize;
    }
    ImVec2 GetSize() const
    {
        return m_viewportSize;
    }

    // Rendering helpers
    void BeginRendering();
    void EndRendering();
    RenderTexture2D GetTexture() const
    {
        return m_renderTexture;
    }

private:
    void UpdateRenderTexture();

    // UI Overlay rendering (editor-only, ImGui-based)
    void RenderUIOverlay();
    void RenderUIElement(const struct UIElementData &elem, int index, ImDrawList *drawList,
                         ImVec2 viewportPos);
    ImVec2 CalculateUIPosition(const struct UIElementData &elem, ImVec2 viewportPos);

    // Helper methods for UI editing
    bool CheckMouseHover(ImVec2 pos, Vector2 size, ImVec2 mousePos);
    void RenderResizeHandles(ImDrawList *drawList, ImVec2 topLeft, ImVec2 bottomRight);
    int GetResizeHandleAtMouse(ImVec2 topLeft, ImVec2 bottomRight, ImVec2 mousePos);

    IEditor *m_editor;
    bool m_visible = true;
    bool m_isHovered = false;
    bool m_isFocused = false;
    ImVec2 m_viewportSize = {800, 600};
    RenderTexture2D m_renderTexture = {0};
    bool m_renderTextureValid = false;

    // UI Dragging state
    bool m_isDraggingUI = false;
    Vector2 m_dragOffset = {0, 0};

    // UI Hover and Resize state
    int m_hoveredUIIndex = -1;
    bool m_isResizingUI = false;
    int m_resizeHandle = -1; // 0-7 for 8 handles, -1 for none
};

#endif // VIEWPORTPANEL_H
