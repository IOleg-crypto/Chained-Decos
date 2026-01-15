#ifndef CH_LAYER_H
#define CH_LAYER_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include <string>


namespace CHEngine
{
class Layer
{
public:
    Layer(const std::string &name = "Layer") : m_DebugName(name)
    {
    }
    virtual ~Layer() = default;

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

protected:
    std::string m_DebugName;
};
} // namespace CHEngine

#endif // CH_LAYER_H
