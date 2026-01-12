#ifndef CH_ENVIRONMENT_PANEL_H
#define CH_ENVIRONMENT_PANEL_H

#include "engine/renderer/render.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class EnvironmentPanel
{
public:
    EnvironmentPanel() = default;
    ~EnvironmentPanel() = default;

    void OnImGuiRender(Scene *scene, bool readOnly, DebugRenderFlags *debugFlags);

private:
    bool m_IsOpen = true;

public:
    bool &IsOpen()
    {
        return m_IsOpen;
    }
};
} // namespace CHEngine

#endif // CH_ENVIRONMENT_PANEL_H
