#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include <variant>
#include <imgui_internal.h>

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

CH_SCRIPT_FUNC void ComboBoxControl_SetSelectedIndex(uint64_t entityID, int index)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<WidgetComponent>())
    {
        auto& widget = entity.GetComponent<WidgetComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            std::get<ComboBoxData>(widget.Data).SelectedIndex = index;
        }
    }
}
CH_ADD_INTERNAL_CALL(ComboBoxControl, ComboBoxControl_SetSelectedIndex_Ptr, ComboBoxControl_SetSelectedIndex);

CH_SCRIPT_FUNC void ComboBoxControl_AddItem(uint64_t entityID, Coral::String item)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<WidgetComponent>())
    {
        auto& widget = entity.GetComponent<WidgetComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            std::get<ComboBoxData>(widget.Data).Items.push_back((std::string)item);
        }
    }
}
CH_ADD_INTERNAL_CALL(ComboBoxControl, ComboBoxControl_AddItem_Ptr, ComboBoxControl_AddItem);

CH_SCRIPT_FUNC void ComboBoxControl_ClearItems(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<WidgetComponent>())
    {
        auto& widget = entity.GetComponent<WidgetComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            auto& combo = std::get<ComboBoxData>(widget.Data);
            combo.Items.clear();
            combo.SelectedIndex = 0;
        }
    }
}
CH_ADD_INTERNAL_CALL(ComboBoxControl, ComboBoxControl_ClearItems_Ptr, ComboBoxControl_ClearItems);

CH_SCRIPT_FUNC int ComboBoxControl_GetItemCount(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<WidgetComponent>())
    {
        auto& widget = entity.GetComponent<WidgetComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            return (int)std::get<ComboBoxData>(widget.Data).Items.size();
        }
    }
    return 0;
}
CH_ADD_INTERNAL_CALL(ComboBoxControl, ComboBoxControl_GetItemCount_Ptr, ComboBoxControl_GetItemCount);

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
    if (ImGui::GetCurrentContext() == nullptr || !ImGui::GetCurrentContext()->WithinFrameScope)
        return;

    CH_CORE_INFO("[UI] UI_Text called from script: '{}'", ((std::string)text));

    auto window = ImGui::GetCurrentContext()->CurrentWindow;
    if (window && !window->SkipItems)
    {
        ImGui::Text("%s", ((std::string)text).c_str());
    }
    else
    {
        // Fallback: draw at top-left of the viewport or screen
        ImGui::GetForegroundDrawList()->AddText({ 10, 10 }, IM_COL32(255, 255, 0, 255), ((std::string)text).c_str());
    }
}
CH_ADD_INTERNAL_CALL(UI, UI_Text_Ptr, UI_Text);

} // namespace CHEngine
