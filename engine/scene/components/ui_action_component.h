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
                CH_HANDLE_NAMED(props, "Target Entity", TargetEntityID);
            }
            CH_PROP_NAMED(props, "Parameter", ParameterName);
            CH_PROP(props, Value);
            const char* typeStrings[] = {"Set Parameter", "Trigger State"};
            CH_ENUM_NAMED(props, "Type", Type, typeStrings);
        CH_REFLECT_END()
    };
}

#endif // CH_UI_ACTION_COMPONENT_H
