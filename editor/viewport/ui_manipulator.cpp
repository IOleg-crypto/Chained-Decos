#include "ui_manipulator.h"
#include "engine/core/log.h"
#include "engine/graphics/ui_renderer.h"
#include "engine/scene/components/control_component.h"

namespace CHEngine
{

static const float HANDLE_SIZE = 8.0f;
static const ImU32 HANDLE_COLOR = IM_COL32(255, 255, 255, 255);
static const ImU32 HANDLE_HOVERED_COLOR = IM_COL32(255, 255, 0, 255);
static const ImU32 ACTIVE_COLOR = IM_COL32(0, 255, 0, 255);

bool EditorUIManipulator::OnImGuiRender(Entity selectedEntity, ImVec2 viewportPos, ImVec2 viewportSize)
{
    if (!selectedEntity || !selectedEntity.HasComponent<ControlComponent>())
    {
        m_Dragging = false;
        m_Resizing = false;
        m_ActiveHandle = UIHandleType::None;
        return false;
    }

    auto& cc = selectedEntity.GetComponent<ControlComponent>();
    Rectangle rect = UIRenderer::Get().GetEntityRect(selectedEntity, viewportSize, viewportPos);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p1 = {rect.x, rect.y};
    ImVec2 p2 = {p1.x + rect.width, p1.y + rect.height};
    ImVec2 center = {p1.x + rect.width * 0.5f, p1.y + rect.height * 0.5f};

    // Draw main frame
    drawList->AddRect(p1, p2, ACTIVE_COLOR, 0, 0, 1.0f);

    ImVec2 mousePos = ImGui::GetMousePos();
    UIHandleType hoveredHandle = UIHandleType::None;

    auto ProcessHandle = [&](UIHandleType type, ImVec2 pos, UIHandleType& hovered) {
        bool hoveredItem = (mousePos.x >= pos.x - HANDLE_SIZE && mousePos.x <= pos.x + HANDLE_SIZE &&
                            mousePos.y >= pos.y - HANDLE_SIZE && mousePos.y <= pos.y + HANDLE_SIZE);
        DrawHandle(drawList, pos, type, hoveredItem || (m_ActiveHandle == type));
        if (hoveredItem && !IsActive())
        {
            hovered = type;
        }
    };

    ProcessHandle(UIHandleType::TopLeft, p1, hoveredHandle);
    ProcessHandle(UIHandleType::TopRight, {p2.x, p1.y}, hoveredHandle);
    ProcessHandle(UIHandleType::BottomLeft, {p1.x, p2.y}, hoveredHandle);
    ProcessHandle(UIHandleType::BottomRight, p2, hoveredHandle);
    ProcessHandle(UIHandleType::Top, {center.x, p1.y}, hoveredHandle);
    ProcessHandle(UIHandleType::Bottom, {center.x, p2.y}, hoveredHandle);
    ProcessHandle(UIHandleType::Left, {p1.x, center.y}, hoveredHandle);
    ProcessHandle(UIHandleType::Right, {p2.x, center.y}, hoveredHandle);

    // Processing interaction
    if (ImGui::IsMouseClicked(0))
    {
        if (hoveredHandle != UIHandleType::None)
        {
            m_Resizing = true;
            m_ActiveHandle = hoveredHandle;
            m_StartMousePos = mousePos;
            m_StartOffsetMin = cc.Transform.OffsetMin;
            m_StartOffsetMax = cc.Transform.OffsetMax;
        }
        else if (ImGui::IsMouseHoveringRect(p1, p2))
        {
            m_Dragging = true;
            m_StartMousePos = mousePos;
            m_StartOffsetMin = cc.Transform.OffsetMin;
            m_StartOffsetMax = cc.Transform.OffsetMax;
        }
    }

    if (IsActive())
    {
        if (ImGui::IsMouseDown(0))
        {
            ImVec2 delta = {mousePos.x - m_StartMousePos.x, mousePos.y - m_StartMousePos.y};

            if (m_Dragging)
            {
                cc.Transform.OffsetMin = {m_StartOffsetMin.x + delta.x, m_StartOffsetMin.y + delta.y};
                cc.Transform.OffsetMax = {m_StartOffsetMax.x + delta.x, m_StartOffsetMax.y + delta.y};
            }
            else if (m_Resizing)
            {
                switch (m_ActiveHandle)
                {
                case UIHandleType::TopLeft:
                    cc.Transform.OffsetMin = {m_StartOffsetMin.x + delta.x, m_StartOffsetMin.y + delta.y};
                    break;
                case UIHandleType::TopRight:
                    cc.Transform.OffsetMin.y = m_StartOffsetMin.y + delta.y;
                    cc.Transform.OffsetMax.x = m_StartOffsetMax.x + delta.x;
                    break;
                case UIHandleType::BottomLeft:
                    cc.Transform.OffsetMin.x = m_StartOffsetMin.x + delta.x;
                    cc.Transform.OffsetMax.y = m_StartOffsetMax.y + delta.y;
                    break;
                case UIHandleType::BottomRight:
                    cc.Transform.OffsetMax = {m_StartOffsetMax.x + delta.x, m_StartOffsetMax.y + delta.y};
                    break;
                case UIHandleType::Top:
                    cc.Transform.OffsetMin.y = m_StartOffsetMin.y + delta.y;
                    break;
                case UIHandleType::Bottom:
                    cc.Transform.OffsetMax.y = m_StartOffsetMax.y + delta.y;
                    break;
                case UIHandleType::Left:
                    cc.Transform.OffsetMin.x = m_StartOffsetMin.x + delta.x;
                    break;
                case UIHandleType::Right:
                    cc.Transform.OffsetMax.x = m_StartOffsetMax.x + delta.x;
                    break;
                }
            }
        }
        else
        {
            m_Dragging = false;
            m_Resizing = false;
            m_ActiveHandle = UIHandleType::None;
        }
        return true;
    }

    return hoveredHandle != UIHandleType::None;
}

void EditorUIManipulator::DrawHandle(ImDrawList* drawList, ImVec2 pos, UIHandleType type, bool hovered)
{
    ImU32 color = hovered ? HANDLE_HOVERED_COLOR : HANDLE_COLOR;
    drawList->AddRectFilled({pos.x - HANDLE_SIZE * 0.5f, pos.y - HANDLE_SIZE * 0.5f},
                            {pos.x + HANDLE_SIZE * 0.5f, pos.y + HANDLE_SIZE * 0.5f}, color);
    drawList->AddRect({pos.x - HANDLE_SIZE * 0.5f, pos.y - HANDLE_SIZE * 0.5f},
                      {pos.x + HANDLE_SIZE * 0.5f, pos.y + HANDLE_SIZE * 0.5f}, IM_COL32(0, 0, 0, 255));
}

} // namespace CHEngine
