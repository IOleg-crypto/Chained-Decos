#ifndef LAYER_H
#define LAYER_H

#include "events/Event.h"
#include <string>

namespace CHEngine
{
class EngineApplication;

class Layer
{
public:
    Layer(const std::string &name = "Layer");
    virtual ~Layer();

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

    void SetAppRunner(EngineApplication *appRunner)
    {
        m_AppRunner = appRunner;
    }
    EngineApplication *GetAppRunner() const
    {
        return m_AppRunner;
    }

protected:
    std::string m_DebugName;
    EngineApplication *m_AppRunner = nullptr;
};

} // namespace CHEngine

#endif // LAYER_H
