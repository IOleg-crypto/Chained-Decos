#ifndef CH_UI_ACTION_COMPONENT_H
#define CH_UI_ACTION_COMPONENT_H

#include "engine/foundation/uuid.h"
#include "engine/reflection/reflection_rfl.h"
#include <string>

namespace Chained
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

        
        struct UI
        {
            UIMeta Type = {.Hint = PropertyMeta::WidgetHint::Enum};
            UIMeta TargetEntityID = {.Tooltip = "ID цільової сутності"};
            UIMeta ParameterName = {.Tooltip = "Ім'я параметра для встановлення"};
            UIMeta Value = {.Speed = 0.1f, .Tooltip = "Значення параметра"};
        };
    };

    CH_MARK_RFL(UIActionComponent);
}

#endif // CH_UI_ACTION_COMPONENT_H
