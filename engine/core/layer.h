#ifndef CH_LAYER_H
#define CH_LAYER_H

#include <string>
#include "engine/foundation/base.h"
#include "engine/core/events/events.h"
#include "engine/foundation/timestep.h"

namespace Chained
{

    // Base class for engine layers with lifecycle callbacks.
    class Layer
    {
    public:
        Layer(const std::string& name = "Layer")
            : m_DebugName(name)
        {
        }
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(Timestep ts) {}
        virtual void OnFixedUpdate(Timestep ts) {}
        virtual void OnRender(Timestep ts) {}
        virtual void OnImGuiRender() {}
        virtual void OnEvent(Event& event) {}

        const std::string& GetName() const { return m_DebugName; }
        

    protected:
        std::string m_DebugName;
    };
} // namespace Chained

#endif // CH_LAYER_H
