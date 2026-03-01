#ifndef CH_UI_MANIPULATOR_H
#define CH_UI_MANIPULATOR_H

#include "engine/scene/scene.h"
#include "imgui.h"
#include "raylib.h"

namespace CHEngine
{

enum class UIHandleType
{
    None = 0,
    Center,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Top,
    Bottom,
    Left,
    Right
};

class EditorUIManipulator
{
public:
    EditorUIManipulator() = default;

    // Returns true if interaction is happening
    bool OnImGuiRender(Entity selectedEntity, ImVec2 viewportPos, ImVec2 viewportSize);

    bool IsActive() const
    {
        return m_Dragging || m_Resizing;
    }

private:
    void DrawHandle(ImDrawList* drawList, ImVec2 pos, UIHandleType type, bool hovered);
    bool HandleInteraction(Entity entity, ImVec2 viewportSize);

private:
    bool m_Dragging = false;
    bool m_Resizing = false;
    UIHandleType m_ActiveHandle = UIHandleType::None;

    // Interaction cache
    ImVec2 m_StartMousePos;
    Vector2 m_StartOffsetMin;
    Vector2 m_StartOffsetMax;
};

} // namespace CHEngine

#endif // CH_UI_MANIPULATOR_H
