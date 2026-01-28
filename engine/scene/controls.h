#ifndef CH_BUTTON_CONTROL_H
#define CH_BUTTON_CONTROL_H

#include "control_base.h"

namespace CHEngine
{
class Button : public Control
{
public:
    Button() : Control("Button")
    {
    }

    virtual void Initialize(Entity entity) override
    {
        Control::Initialize(entity);
        if (!entity.HasComponent<ButtonControl>())
            entity.AddComponent<ButtonControl>();
    }
};

class Panel : public Control
{
public:
    Panel() : Control("Panel")
    {
    }

    virtual void Initialize(Entity entity) override
    {
        Control::Initialize(entity);
        if (!entity.HasComponent<PanelControl>())
            entity.AddComponent<PanelControl>();
    }
};

class CheckBox : public Control
{
public:
    CheckBox() : Control("CheckBox")
    {
    }

    virtual void Initialize(Entity entity) override
    {
        Control::Initialize(entity);
        if (!entity.HasComponent<CheckboxControl>())
            entity.AddComponent<CheckboxControl>();
    }
};

class Slider : public Control
{
public:
    Slider() : Control("Slider")
    {
    }

    virtual void Initialize(Entity entity) override
    {
        Control::Initialize(entity);
        if (!entity.HasComponent<SliderControl>())
            entity.AddComponent<SliderControl>();
    }
};
} // namespace CHEngine

#endif // CH_BUTTON_CONTROL_H
