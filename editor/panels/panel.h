#ifndef CH_PANEL_H
#define CH_PANEL_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/core/timestep.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
// Base class for dockable editor panels with optional scene context.
class Panel
{
public:
    virtual ~Panel() = default;

    // Draws the panel UI. readOnly is used when the panel should avoid editing.
    virtual void OnImGuiRender(bool readOnly = false) = 0;
    // Optional per-frame update hook.
    virtual void OnUpdate(Timestep ts)
    {
    }
    // Optional event hook.
    virtual void OnEvent(Event& e)
    {
    }
    // Optional configuration hook used by settings panels.
    virtual void OnConfiguration()
    {
    }
    // Updates the scene context used by the panel.
    virtual void SetContext(const std::shared_ptr<Scene>& context)
    {
        m_Context = context;
    }

    bool& IsOpen()
    {
        return m_IsOpen;
    }
    bool& ShowSettings()
    {
        return m_ShowSettings;
    }
    const std::string& GetName() const
    {
        return m_Name;
    }

protected:
    std::string m_Name;
    std::shared_ptr<Scene> m_Context;
    bool m_IsOpen = true;
    bool m_ShowSettings = false;
};
} // namespace CHEngine

#endif // CH_PANEL_H
