#ifndef CH_UI_ACTION_COMPONENT_H
#define CH_UI_ACTION_COMPONENT_H

#include "engine/core/uuid.h"
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

        UIActionComponent() = default;

        CH_REFLECT_BEGIN(UIActionComponent)
            if (props.GetMode() != CHEngine::ReflectionMode::UI){
                props.Handle("Target Entity", TargetEntityID);
            }
            props.Property("Parameter", ParameterName);
            props.Property("Value", Value);
            const char* typeStrings[] = {"Set Parameter", "Trigger State"};
            props.Enum("Type", Type, typeStrings, 2);
        CH_REFLECT_END()
    };
}

#endif // CH_UI_ACTION_COMPONENT_H
