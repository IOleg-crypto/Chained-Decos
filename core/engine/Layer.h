#ifndef LAYER_H
#define LAYER_H

#include <string>

namespace ChainedDecos
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
    virtual void OnEvent()
    {
    } // In Hazel, this takes an Event&, for now we keep it empty or integrate with existing
      // EventBus

    inline const std::string &GetName() const
    {
        return m_DebugName;
    }

protected:
    std::string m_DebugName;
};

} // namespace ChainedDecos

#endif // LAYER_H
