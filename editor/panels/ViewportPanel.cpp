//
// ViewportPanel.cpp - 3D viewport panel implementation
//

#include "ViewportPanel.h"
#include "editor/IEditor.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/UIComponents.h"
#include "scene/ecs/systems/UIRenderSystem.h"
#include <cmath>
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
    Color bgColor =
        m_editor ? m_editor->GetSceneManager().GetGameScene().GetMapMetaData().backgroundColor
                 : DARKGRAY;
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
            if (m_editor->GetState().IsUIDesignMode() && !m_editor->IsInPlayMode())
            {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 itemPos = ImGui::GetItemRectMin(); // Top-left of the viewport image
                Vector2 localMousePos = {mousePos.x - itemPos.x, mousePos.y - itemPos.y};

                if (m_isHovered && ImGui::IsMouseClicked(0))
                {
                    auto picked = CHEngine::UIRenderSystem::PickUIEntity(
                        localMousePos, (int)m_viewportSize.x, (int)m_viewportSize.y);

                    if (picked != entt::null)
                    {
                        auto &registry = ECSRegistry::Get();
                        if (registry.all_of<CHEngine::UIElementIndex>(picked))
                        {
                            int index = registry.get<CHEngine::UIElementIndex>(picked).index;
                            m_editor->GetSelectionManager().SelectUIElement(index);

                            // Start dragging
                            m_isDraggingUI = true;
                            auto &transform = registry.get<CHEngine::RectTransform>(picked);
                            m_dragOffset = Vector2Subtract(transform.position, localMousePos);
                        }
                    }
                    else
                    {
                        m_editor->GetSelectionManager().SelectUIElement(-1);
                    }
                }

                if (m_isDraggingUI)
                {
                    if (ImGui::IsMouseDown(0))
                    {
                        int selectedIdx =
                            m_editor->GetSelectionManager().GetSelectedUIElementIndex();
                        if (selectedIdx != -1)
                        {
                            auto &uiElements =
                                m_editor->GetSceneManager().GetGameScene().GetUIElementsMutable();
                            if (selectedIdx < (int)uiElements.size())
                            {
                                Vector2 newPos = Vector2Add(localMousePos, m_dragOffset);
                                uiElements[selectedIdx].position = newPos;
                                m_editor->GetSceneManager().SetSceneModified(true);

                                // Refresh ECS for immediate feedback
                                m_editor->GetSelectionManager().RefreshUIEntities();
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

        // Render UI overlay if in UI design mode
        if (m_editor && m_editor->GetState().IsUIDesignMode())
        {
            RenderUIOverlay();
        }
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

void ViewportPanel::RenderUIOverlay()
{
    if (!m_editor)
        return;

    // Get viewport position - the image we just drew
    ImVec2 viewportPos = ImGui::GetItemRectMin();

    // Use window's draw list to draw on top
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    // Get mouse position for hover detection
    ImVec2 mousePos = ImGui::GetMousePos();
    m_hoveredUIIndex = -1; // Reset hover state

    // Render all UI elements and check for hover
    auto &uiElements = m_editor->GetSceneManager().GetGameScene().GetUIElements();
    for (int i = 0; i < (int)uiElements.size(); i++)
    {
        if (!uiElements[i].isActive)
            continue;

        // Check if mouse is hovering over this element
        ImVec2 elemPos = CalculateUIPosition(uiElements[i], viewportPos);
        if (CheckMouseHover(elemPos, uiElements[i].size, mousePos))
        {
            m_hoveredUIIndex = i;
        }

        RenderUIElement(uiElements[i], i, drawList, viewportPos);
    }
}

void ViewportPanel::RenderUIElement(const UIElementData &elem, int index, ImDrawList *drawList,
                                    ImVec2 viewportPos)
{
    bool isSelected = (m_editor->GetSelectionManager().GetSelectedUIElementIndex() == index);
    bool isHovered = (m_hoveredUIIndex == index);

    // Calculate position based on anchor
    ImVec2 pos = CalculateUIPosition(elem, viewportPos);
    ImVec2 topLeft = pos;
    ImVec2 bottomRight = ImVec2(pos.x + elem.size.x, pos.y + elem.size.y);

    // Draw hover highlight if hovered (but not selected)
    if (isHovered && !isSelected)
    {
        // Light blue hover border
        drawList->AddRect(topLeft, bottomRight, IM_COL32(100, 150, 255, 180), 0.0f, 0, 2.0f);
    }

    // Draw selection outline if selected
    if (isSelected)
    {
        // Yellow selection border
        drawList->AddRect(topLeft, bottomRight, IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f);

        // Draw anchor point (small circle)
        ImVec2 anchorPoint = CalculateUIPosition(elem, viewportPos);
        drawList->AddCircleFilled(anchorPoint, 5.0f, IM_COL32(255, 255, 0, 200));

        // Draw resize handles
        RenderResizeHandles(drawList, topLeft, bottomRight);
    }

    // Render based on type (Content is now handled by UIRenderSystem/ImGui)
}

ImVec2 ViewportPanel::CalculateUIPosition(const UIElementData &elem, ImVec2 viewportPos)
{
    ImVec2 viewportSize = m_viewportSize;
    ImVec2 pos = ImVec2(elem.position.x, elem.position.y);

    // Apply anchor offset
    switch (elem.anchor)
    {
    case 0: // TopLeft
        // No offset needed
        break;
    case 1: // TopCenter
        pos.x += viewportSize.x * 0.5f;
        break;
    case 2: // TopRight
        pos.x += viewportSize.x;
        break;
    case 3: // MiddleLeft
        pos.y += viewportSize.y * 0.5f;
        break;
    case 4: // MiddleCenter
        pos.x += viewportSize.x * 0.5f;
        pos.y += viewportSize.y * 0.5f;
        break;
    case 5: // MiddleRight
        pos.x += viewportSize.x;
        pos.y += viewportSize.y * 0.5f;
        break;
    case 6: // BottomLeft
        pos.y += viewportSize.y;
        break;
    case 7: // BottomCenter
        pos.x += viewportSize.x * 0.5f;
        pos.y += viewportSize.y;
        break;
    case 8: // BottomRight
        pos.x += viewportSize.x;
        pos.y += viewportSize.y;
        break;
    }

    // Apply pivot offset
    pos.x -= elem.size.x * elem.pivot.x;
    pos.y -= elem.size.y * elem.pivot.y;

    // Add viewport position offset (passed as parameter)
    pos.x += viewportPos.x;
    pos.y += viewportPos.y;

    return pos;
}

bool ViewportPanel::CheckMouseHover(ImVec2 pos, Vector2 size, ImVec2 mousePos)
{
    return (mousePos.x >= pos.x && mousePos.x <= pos.x + size.x && mousePos.y >= pos.y &&
            mousePos.y <= pos.y + size.y);
}

void ViewportPanel::RenderResizeHandles(ImDrawList *drawList, ImVec2 topLeft, ImVec2 bottomRight)
{
    const float handleSize = 8.0f;
    const ImU32 handleColor = IM_COL32(255, 255, 255, 255);
    const ImU32 handleBorder = IM_COL32(0, 0, 0, 255);

    ImVec2 topRight = ImVec2(bottomRight.x, topLeft.y);
    ImVec2 bottomLeft = ImVec2(topLeft.x, bottomRight.y);
    ImVec2 middleTop = ImVec2((topLeft.x + bottomRight.x) * 0.5f, topLeft.y);
    ImVec2 middleBottom = ImVec2((topLeft.x + bottomRight.x) * 0.5f, bottomRight.y);
    ImVec2 middleLeft = ImVec2(topLeft.x, (topLeft.y + bottomRight.y) * 0.5f);
    ImVec2 middleRight = ImVec2(bottomRight.x, (topLeft.y + bottomRight.y) * 0.5f);

    // Draw 8 resize handles (corners + edges)
    ImVec2 handles[8] = {
        topLeft,      // 0: Top-Left
        middleTop,    // 1: Top
        topRight,     // 2: Top-Right
        middleRight,  // 3: Right
        bottomRight,  // 4: Bottom-Right
        middleBottom, // 5: Bottom
        bottomLeft,   // 6: Bottom-Left
        middleLeft    // 7: Left
    };

    for (int i = 0; i < 8; i++)
    {
        ImVec2 handleMin =
            ImVec2(handles[i].x - handleSize * 0.5f, handles[i].y - handleSize * 0.5f);
        ImVec2 handleMax =
            ImVec2(handles[i].x + handleSize * 0.5f, handles[i].y + handleSize * 0.5f);

        // Draw handle background
        drawList->AddRectFilled(handleMin, handleMax, handleColor);
        // Draw handle border
        drawList->AddRect(handleMin, handleMax, handleBorder, 0.0f, 0, 1.0f);
    }
}

int ViewportPanel::GetResizeHandleAtMouse(ImVec2 topLeft, ImVec2 bottomRight, ImVec2 mousePos)
{
    const float handleSize = 8.0f;
    const float tolerance = handleSize * 0.5f;

    ImVec2 topRight = ImVec2(bottomRight.x, topLeft.y);
    ImVec2 bottomLeft = ImVec2(topLeft.x, bottomRight.y);
    ImVec2 middleTop = ImVec2((topLeft.x + bottomRight.x) * 0.5f, topLeft.y);
    ImVec2 middleBottom = ImVec2((topLeft.x + bottomRight.x) * 0.5f, bottomRight.y);
    ImVec2 middleLeft = ImVec2(topLeft.x, (topLeft.y + bottomRight.y) * 0.5f);
    ImVec2 middleRight = ImVec2(bottomRight.x, (topLeft.y + bottomRight.y) * 0.5f);

    ImVec2 handles[8] = {topLeft,     middleTop,    topRight,   middleRight,
                         bottomRight, middleBottom, bottomLeft, middleLeft};

    for (int i = 0; i < 8; i++)
    {
        if (fabs(mousePos.x - handles[i].x) <= tolerance &&
            fabs(mousePos.y - handles[i].y) <= tolerance)
        {
            return i;
        }
    }

    return -1; // No handle at mouse position
}
