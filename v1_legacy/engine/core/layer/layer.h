#ifndef CD_ENGINE_CORE_LAYER_LAYER_H
#define CD_ENGINE_CORE_LAYER_LAYER_H

#include "events/event.h"
#include <string>

namespace CHEngine
{
class Application;

class Layer
{
public:
    Layer(const std::string &name = "Layer") : m_DebugName(name)
    {
    }
    virtual ~Layer()
    {
    }

    virtual void OnAttach()
    {
    }
    virtual void OnDetach()
    {
    }
    virtual void OnUpdate(float deltaTime)
    {
    }
    virtual void OnRender()
    {
    }
    virtual void OnImGuiRender()
    {
    }
    virtual void OnEvent(Event &event)
    {
    }

    const std::string &GetName() const
    {
        return m_DebugName;
    }
    void SetAppRunner(Application *appRunner)
    {
        m_AppRunner = appRunner;
    }
    Application *GetAppRunner() const
    {
        return m_AppRunner;
    }

protected:
    std::string m_DebugName;
    Application *m_AppRunner = nullptr;
};
} // namespace CHEngine

#endif
