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
};

#endif // VIEWPORTPANEL_H
