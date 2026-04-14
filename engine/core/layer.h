#ifndef CH_LAYER_H
#define CH_LAYER_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/core/timestep.h"
#include <string>

namespace CHEngine
{
// Base class for engine layers with lifecycle callbacks and an enable flag.
class Layer
{
public:
    Layer(const std::string& name = "Layer")
        : m_DebugName(name)
    {
    }
    virtual ~Layer() = default;

    virtual void OnAttach()
    {
    }
    virtual void OnDetach()
    {
    }

    // Returns whether the layer is currently enabled.
    bool IsEnabled() const
    {
        return m_Enabled;
    }
    // Enables or disables the layer without destroying it.
    void SetEnabled(bool enabled)
    {
        m_Enabled = enabled;
    }

    virtual void OnUpdate(Timestep ts)
    {
    }
    virtual void OnFixedUpdate(Timestep ts)
    {
    }
    virtual void OnRender(Timestep ts)
    {
    }
    virtual void OnImGuiRender()
    {
    }
    virtual void OnEvent(Event& event)
    {
    }

    const std::string& GetName() const
    {
        return m_DebugName;
    }

protected:
    std::string m_DebugName;
    bool m_Enabled = true;
};
} // namespace CHEngine

#endif // CH_LAYER_H
