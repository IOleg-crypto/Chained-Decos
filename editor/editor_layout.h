#ifndef CH_EDITOR_LAYOUT_H
#define CH_EDITOR_LAYOUT_H

#include "editor/editor_panels.h"
#include "engine/core/events.h"
#include <functional>

namespace CHEngine
{

class EditorLayout
{
public:
    void BeginWorkspace();
    void EndWorkspace();

    void DrawInterface();
    void ResetLayout();
    void SaveDefaultLayout();

private:
    void DrawProjectSelector();
};

} // namespace CHEngine

#endif // CH_EDITOR_LAYOUT_H
