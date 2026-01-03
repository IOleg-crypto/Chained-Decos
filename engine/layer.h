#ifndef CH_LAYER_H
#define CH_LAYER_H

#include "engine/events.h"
#include "engine/types.h"
#include <string>

namespace CH
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
} // namespace CH

#endif // CH_LAYER_H
