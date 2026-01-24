#ifndef CH_PANEL_H
#define CH_PANEL_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class Panel
{
public:
    virtual ~Panel() = default;

    virtual void OnImGuiRender(bool readOnly = false) = 0;
    virtual void OnUpdate(float deltaTime)
    {
    }
    virtual void OnEvent(Event &e)
    {
    }
    virtual void SetContext(const Ref<Scene> &context)
    {
        m_Context = context;
    }

    bool &IsOpen()
    {
        return m_IsOpen;
    }
    const std::string &GetName() const
    {
        return m_Name;
    }

protected:
    std::string m_Name;
    Ref<Scene> m_Context;
    bool m_IsOpen = true;
};
} // namespace CHEngine

#endif // CH_PANEL_H
