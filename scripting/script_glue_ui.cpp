#include "script_glue_ui.h"

namespace Chained {
bool ButtonControl_IsClicked(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<UIControlComponent>()
               ? entity.GetComponent<UIControlComponent>().PressedThisFrame
               : false;
}
bool ButtonControl_IsDown(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<UIControlComponent>() ? entity.GetComponent<UIControlComponent>().IsDown
                                                               : false;
}
bool CheckboxControl_GetChecked(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<UIControlComponent>())
    {
        auto& widget = entity.GetComponent<UIControlComponent>();
        if (std::holds_alternative<CheckboxData>(widget.Data))
        {
            return std::get<CheckboxData>(widget.Data).Checked;
        }
    }
    return false;
}
int ComboBoxControl_GetSelectedIndex(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<UIControlComponent>())
    {
        auto& widget = entity.GetComponent<UIControlComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            return std::get<ComboBoxData>(widget.Data).SelectedIndex;
        }
    }
    return 0;
}
void ComboBoxControl_SetSelectedIndex(uint64_t entityID, int index)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<UIControlComponent>())
    {
        auto& widget = entity.GetComponent<UIControlComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            std::get<ComboBoxData>(widget.Data).SelectedIndex = index;
        }
    }
}
void ComboBoxControl_AddItem(uint64_t entityID, const char16_t* item)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<UIControlComponent>() && item)
    {
        auto& widget = entity.GetComponent<UIControlComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            std::get<ComboBoxData>(widget.Data).Items.push_back(ch_u16_to_string(item));
        }
    }
}
void ComboBoxControl_ClearItems(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<UIControlComponent>())
    {
        auto& widget = entity.GetComponent<UIControlComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            auto& combo = std::get<ComboBoxData>(widget.Data);
            combo.Items.clear();
            combo.SelectedIndex = 0;
        }
    }
}
int ComboBoxControl_GetItemCount(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<UIControlComponent>())
    {
        auto& widget = entity.GetComponent<UIControlComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            return (int)std::get<ComboBoxData>(widget.Data).Items.size();
        }
    }
    return 0;
}
static thread_local std::u16string s_UITagBuffer;

const char16_t* ComboBoxControl_GetItem(uint64_t entityID, int index)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<UIControlComponent>())
    {
        auto& widget = entity.GetComponent<UIControlComponent>();
        if (std::holds_alternative<ComboBoxData>(widget.Data))
        {
            auto& combo = std::get<ComboBoxData>(widget.Data);
            if (index >= 0 && index < (int)combo.Items.size())
            {
                s_UITagBuffer = ch_utf8_to_u16(combo.Items[index]);
                return s_UITagBuffer.c_str();
            }
        }
    }
    return nullptr;
}
void UI_Text(const char16_t* text)
{
    if (ImGui::GetCurrentContext() == nullptr || !ImGui::GetCurrentContext()->WithinFrameScope || !text)
    {
        return;
    }

    std::string strText = ch_u16_to_string(text);
    CH_CORE_INFO("[UI] UI_Text called from script: '{}'", strText);

    auto window = ImGui::GetCurrentContext()->CurrentWindow;
    if (window && !window->SkipItems)
    {
        ImGui::Text("%s", strText.c_str());
    }
    else
    {
        ImGui::GetForegroundDrawList()->AddText({10, 10}, IM_COL32(255, 255, 0, 255), strText.c_str());
    }
}

void RegisterGlueUI()
{
    CH_ADD_INTERNAL_CALL("UI", ButtonControl_IsClicked, ButtonControl_IsClicked);
    CH_ADD_INTERNAL_CALL("UI", ButtonControl_IsDown, ButtonControl_IsDown);
    CH_ADD_INTERNAL_CALL("UI", CheckboxControl_GetChecked, CheckboxControl_GetChecked);
    CH_ADD_INTERNAL_CALL("UI", ComboBoxControl_GetSelectedIndex, ComboBoxControl_GetSelectedIndex);
    CH_ADD_INTERNAL_CALL("UI", ComboBoxControl_SetSelectedIndex, ComboBoxControl_SetSelectedIndex);
    CH_ADD_INTERNAL_CALL("UI", ComboBoxControl_AddItem, ComboBoxControl_AddItem);
    CH_ADD_INTERNAL_CALL("UI", ComboBoxControl_ClearItems, ComboBoxControl_ClearItems);
    CH_ADD_INTERNAL_CALL("UI", ComboBoxControl_GetItemCount, ComboBoxControl_GetItemCount);
    CH_ADD_INTERNAL_CALL("UI", ComboBoxControl_GetItem, ComboBoxControl_GetItem);
    CH_ADD_INTERNAL_CALL("UI", UI_Text, UI_Text);
}
}
