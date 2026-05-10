#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include <variant>

namespace CHEngine
{

void RegisterGlueUI() {}

// ── UI Controls ───────────────────────────────────────────────────────
CH_SCRIPT_FUNC bool ButtonControl_IsPressed(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<WidgetComponent>() ? entity.GetComponent<WidgetComponent>().PressedThisFrame
                                                          : false;
}
CH_ADD_INTERNAL_CALL(ButtonControl, ButtonControl_IsPressed_Ptr, ButtonControl_IsPressed);

CH_SCRIPT_FUNC bool CheckboxControl_GetChecked(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<WidgetComponent>())
    {
        auto& widget = entity.GetComponent<WidgetComponent>();
        if (std::holds_alternative<CheckboxData>(widget.Data))
            return std::get<CheckboxData>(widget.Data).Checked;
    }
    return false;
}
CH_ADD_INTERNAL_CALL(CheckboxControl, CheckboxControl_GetChecked_Ptr, CheckboxControl_GetChecked);

CH_SCRIPT_FUNC int ComboBoxControl_GetSelectedIndex(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<WidgetComponent>())
    {
        auto& widget = entity.GetComponent<WidgetComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
            return std::get<ComboBoxData>(widget.Data).SelectedIndex;
    }
    return 0;
}
CH_ADD_INTERNAL_CALL(ComboBoxControl, ComboBoxControl_GetSelectedIndex_Ptr, ComboBoxControl_GetSelectedIndex);

CH_SCRIPT_FUNC Coral::String ComboBoxControl_GetItem(uint64_t entityID, int index)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<WidgetComponent>())
    {
        auto& widget = entity.GetComponent<WidgetComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            auto& combo = std::get<ComboBoxData>(widget.Data);
            if (index >= 0 && index < (int)combo.Items.size())
            {
                return Coral::String::New(combo.Items[index]);
            }
        }
    }
    return Coral::String::New("");
}
CH_ADD_INTERNAL_CALL(ComboBoxControl, ComboBoxControl_GetItem_Ptr, ComboBoxControl_GetItem);

CH_SCRIPT_FUNC void UI_Text(Coral::String text)
{
    ImGui::Text("%s", ((std::string)text).c_str());
}
CH_ADD_INTERNAL_CALL(UI, UI_Text_Ptr, UI_Text);

} // namespace CHEngine
