#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include <variant>
#include <imgui_internal.h>

namespace Chained
{



// ── UI Controls ───────────────────────────────────────────────────────
CH_SCRIPT_FUNC bool ButtonControl_IsClicked(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<WidgetComponent>() ? entity.GetComponent<WidgetComponent>().PressedThisFrame
                                                          : false;
}


CH_SCRIPT_FUNC bool ButtonControl_IsDown(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<WidgetComponent>() ? entity.GetComponent<WidgetComponent>().IsDown
                                                           : false;
}


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


    void RegisterGlueUI(Coral::ManagedAssembly& assembly) {
            assembly.AddInternalCall("Chained.ButtonControl", "ButtonControl_IsClicked_Ptr", (void*)ButtonControl_IsClicked);
            assembly.AddInternalCall("Chained.ButtonControl", "ButtonControl_IsDown_Ptr", (void*)ButtonControl_IsDown);
            assembly.AddInternalCall("Chained.CheckboxControl", "CheckboxControl_GetChecked_Ptr", (void*)CheckboxControl_GetChecked);
            assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_GetSelectedIndex_Ptr", (void*)ComboBoxControl_GetSelectedIndex);
            assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_SetSelectedIndex_Ptr", (void*)ComboBoxControl_SetSelectedIndex);
            assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_AddItem_Ptr", (void*)ComboBoxControl_AddItem);
            assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_ClearItems_Ptr", (void*)ComboBoxControl_ClearItems);
            assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_GetItemCount_Ptr", (void*)ComboBoxControl_GetItemCount);
            assembly.AddInternalCall("Chained.ComboBoxControl", "ComboBoxControl_GetItem_Ptr", (void*)ComboBoxControl_GetItem);
            assembly.AddInternalCall("Chained.UI", "UI_Text_Ptr", (void*)UI_Text);
        }
} // namespace Chained
