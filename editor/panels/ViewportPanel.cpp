//
// ViewportPanel.cpp - 3D viewport panel implementation
//

#include "ViewportPanel.h"
#include "editor/IEditor.h"
#include <imgui.h>
#include <rlgl.h>

ViewportPanel::ViewportPanel(IEditor *editor) : m_editor(editor)
{
}

ViewportPanel::~ViewportPanel()
{
    if (m_renderTextureValid && m_renderTexture.id != 0)
    {
        UnloadRenderTexture(m_renderTexture);
    }
}

void ViewportPanel::Update(float deltaTime)
{
    // Viewport-specific updates can go here
}

void ViewportPanel::UpdateRenderTexture()
{
    int width = static_cast<int>(m_viewportSize.x);
    int height = static_cast<int>(m_viewportSize.y);

    if (width <= 0 || height <= 0)
        return;

    // Only recreate if size changed
    if (m_renderTextureValid && m_renderTexture.texture.width == width &&
        m_renderTexture.texture.height == height)
        return;

    // Unload old texture
    if (m_renderTextureValid && m_renderTexture.id != 0)
    {
        UnloadRenderTexture(m_renderTexture);
    }

    // Create new render texture
    m_renderTexture = LoadRenderTexture(width, height);
    m_renderTextureValid = (m_renderTexture.id != 0);
}

void ViewportPanel::BeginRendering()
{
    if (!m_renderTextureValid || m_renderTexture.id == 0)
        return;

    BeginTextureMode(m_renderTexture);
    ClearBackground(m_editor ? m_editor->GetClearColor() : DARKGRAY);
}

void ViewportPanel::EndRendering()
{
    if (m_renderTextureValid && m_renderTexture.id != 0)
    {
        EndTextureMode();
    }
}

void ViewportPanel::Render()
{
    if (!m_visible)
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::Begin("Viewport", &m_visible,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        m_isHovered = ImGui::IsWindowHovered();
        m_isFocused = ImGui::IsWindowFocused();

        // Get available size for viewport
        ImVec2 availableSize = ImGui::GetContentRegionAvail();

        // Minimum viewport size
        if (availableSize.x < 100)
            availableSize.x = 100;
        if (availableSize.y < 100)
            availableSize.y = 100;

        // Update if size changed
        if (availableSize.x != m_viewportSize.x || availableSize.y != m_viewportSize.y)
        {
            m_viewportSize = availableSize;
            UpdateRenderTexture();
        }

        // Display the render texture (flipped vertically for OpenGL)
        if (m_renderTextureValid)
        {
            // Flip UV vertically because OpenGL texture coordinates are inverted
            ImGui::Image((ImTextureID)(intptr_t)m_renderTexture.texture.id, m_viewportSize,
                         ImVec2(0, 1), // UV0: top-left
                         ImVec2(1, 0)  // UV1: bottom-right (flipped)
            );
        }
        else
        {
            ImGui::TextDisabled("Viewport not initialized");
        }
    }
    ImGui::End();

    ImGui::PopStyleVar();
}
