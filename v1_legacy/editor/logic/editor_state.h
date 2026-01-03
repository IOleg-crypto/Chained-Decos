#ifndef CD_EDITOR_LOGIC_EDITOR_STATE_H
#define CD_EDITOR_LOGIC_EDITOR_STATE_H

#include "editor/camera/editor_camera.h"
#include "editor/editor_types.h"

namespace CHEngine
{
class EditorState
{
public:
    static EditorState &Get();
    static void Init();
    static void Shutdown();

    EditorState() = default;
    ~EditorState() = default;

    EditorCamera &GetCamera()
    {
        return m_Camera;
    }
    Tool &GetActiveTool()
    {
        return m_ActiveTool;
    }
    void SetActiveTool(Tool tool)
    {
        m_ActiveTool = tool;
    }

private:
    EditorCamera m_Camera;
    Tool m_ActiveTool = Tool::Select;

    static EditorState *s_Instance;
};
} // namespace CHEngine

#endif // CD_EDITOR_LOGIC_EDITOR_STATE_H
