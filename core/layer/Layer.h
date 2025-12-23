#ifndef LAYER_H
#define LAYER_H

#include "events/Event.h"
#include <string>

namespace CHEngine
{

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
    virtual void OnEvent(Event &event)
    {
    }

    const std::string &GetName() const
    {
        return m_DebugName;
    }

protected:
    std::string m_DebugName;
};

} // namespace CHEngine

#endif // LAYER_H
