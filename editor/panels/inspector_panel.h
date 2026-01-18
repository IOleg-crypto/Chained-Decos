#ifndef CH_INSPECTOR_PANEL_H
#define CH_INSPECTOR_PANEL_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/renderer/render.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class InspectorPanel
{
public:
    InspectorPanel() = default;
    void OnImGuiRender(Scene *scene, Entity entity, bool readOnly = false);
    void SetSelectedMeshIndex(int index)
    {
        m_SelectedMeshIndex = index;
    }

private:
    void DrawComponents(Entity entity);

private:
    int m_SelectedMeshIndex = -1;
};
} // namespace CHEngine

#endif // CH_INSPECTOR_PANEL_H
