#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/core/timestep.h"
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

    bool IsEnabled() const
    {
        return m_Enabled;
    }
    void SetEnabled(bool enabled)
    {
        m_Enabled = enabled;
    }

    virtual void OnUpdate(float deltaTime)
    {
    }
    virtual void OnRender(Timestep ts)
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
    bool m_Enabled = true;
};
} // namespace CHEngine

#endif // CH_LAYER_H
