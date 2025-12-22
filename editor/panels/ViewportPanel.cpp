//
// ViewportPanel.cpp - 3D viewport panel implementation
//

#include "ViewportPanel.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/UIComponents.h"
#include "scene/ecs/systems/UIRenderSystem.h"
#include "editor/IEditor.h"
#include <imgui.h>
#include <raymath.h>
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
    // Use background color from scene metadata
    Color bgColor = m_editor ? m_editor->GetGameScene().GetMapMetaData().backgroundColor : DARKGRAY;
    ClearBackground(bgColor);
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

            // UI Mode: Handle clicking to select elements and dragging
            if (m_editor->IsUIDesignMode() && !m_editor->IsInPlayMode())
            {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 itemPos = ImGui::GetItemRectMin(); // Top-left of the viewport image
                Vector2 localMousePos = {mousePos.x - itemPos.x, mousePos.y - itemPos.y};

                if (m_isHovered && ImGui::IsMouseClicked(0))
                {
                    auto picked = ChainedDecos::UIRenderSystem::PickUIEntity(
                        localMousePos, (int)m_viewportSize.x, (int)m_viewportSize.y);

                    if (picked != entt::null)
                    {
                        auto &registry = ECSRegistry::Get();
                        if (registry.all_of<ChainedDecos::UIElementIndex>(picked))
                        {
                            int index = registry.get<ChainedDecos::UIElementIndex>(picked).index;
                            m_editor->SelectUIElement(index);

                            // Start dragging
                            m_isDraggingUI = true;
                            auto &transform = registry.get<ChainedDecos::RectTransform>(picked);
                            m_dragOffset = Vector2Subtract(transform.position, localMousePos);
                        }
                    }
                    else
                    {
                        m_editor->SelectUIElement(-1);
                    }
                }

                if (m_isDraggingUI)
                {
                    if (ImGui::IsMouseDown(0))
                    {
                        int selectedIdx = m_editor->GetSelectedUIElementIndex();
                        if (selectedIdx != -1)
                        {
                            auto &uiElements = m_editor->GetGameScene().GetUIElementsMutable();
                            if (selectedIdx < (int)uiElements.size())
                            {
                                Vector2 newPos = Vector2Add(localMousePos, m_dragOffset);
                                uiElements[selectedIdx].position = newPos;
                                m_editor->SetSceneModified(true);

                                // Refresh ECS for immediate feedback
                                m_editor->RefreshUIEntities();
                            }
                        }
                    }
                    else
                    {
                        m_isDraggingUI = false;
                    }
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Viewport not initialized");
        }
    }
    ImGui::End();

    ImGui::PopStyleVar();
}
