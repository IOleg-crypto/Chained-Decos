#ifndef CH_INSPECTOR_PANEL_H
#define CH_INSPECTOR_PANEL_H

#include "engine/core/base.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene.h"

namespace CH
{
class InspectorPanel
{
public:
    InspectorPanel() = default;

    void OnImGuiRender(Scene *scene, Entity entity);

private:
    void DrawComponents(Entity entity);
};
} // namespace CH

#endif // CH_INSPECTOR_PANEL_H
