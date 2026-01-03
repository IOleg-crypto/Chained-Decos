#ifndef CH_INSPECTOR_PANEL_H
#define CH_INSPECTOR_PANEL_H

#include "engine/base.h"
#include "engine/entity.h"
#include "engine/scene.h"


namespace CH
{
class InspectorPanel
{
public:
    InspectorPanel() = default;

    void OnImGuiRender(Entity entity);

private:
    void DrawComponents(Entity entity);
};
} // namespace CH

#endif // CH_INSPECTOR_PANEL_H
