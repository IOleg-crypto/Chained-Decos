#ifndef CH_ENVIRONMENT_PANEL_H
#define CH_ENVIRONMENT_PANEL_H

#include "engine/scene/scene.h"

namespace CH
{
class EnvironmentPanel
{
public:
    EnvironmentPanel() = default;
    ~EnvironmentPanel() = default;

    void OnImGuiRender(Scene *scene);

private:
    bool m_IsOpen = true;

public:
    bool &IsOpen()
    {
        return m_IsOpen;
    }
};
} // namespace CH

#endif // CH_ENVIRONMENT_PANEL_H
