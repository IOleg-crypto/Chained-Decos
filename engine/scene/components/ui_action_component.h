#ifndef CH_UI_ACTION_COMPONENT_H
#define CH_UI_ACTION_COMPONENT_H

#include "engine/core/uuid.h"
#include "engine/core/reflection_rfl.h"
#include <string>

namespace CHEngine
{
    enum class UIActionType
    {
        SetParameter,
        TriggerState
    };

    struct UIActionComponent
    {
        UUID TargetEntityID; // Entity with AnimationGraphComponent
        std::string ParameterName;
        float Value;
        UIActionType Type = UIActionType::SetParameter;

        static const char* GetStaticName() { return "UIActionComponent"; }
    };

    CH_MARK_RFL(UIActionComponent);
}

#endif // CH_UI_ACTION_COMPONENT_H
