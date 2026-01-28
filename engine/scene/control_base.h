#ifndef CH_CONTROL_BASE_H
#define CH_CONTROL_BASE_H

#include "components/control_component.h"
#include "control.h"


namespace CHEngine
{
class Control : public IControl
{
public:
    Control(const std::string &name) : m_Name(name)
    {
    }
    virtual ~Control() = default;

    virtual void Initialize(Entity entity) override
    {
        if (!entity.HasComponent<ControlComponent>())
            entity.AddComponent<ControlComponent>();
    }

    virtual const std::string &GetName() const override
    {
        return m_Name;
    }

protected:
    std::string m_Name;
};
} // namespace CHEngine

#endif // CH_CONTROL_BASE_H
