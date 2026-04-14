#include "script_glue_internal.h"

namespace CHEngine
{

// ── UI Controls ───────────────────────────────────────────────────────
CH_SCRIPT_FUNC bool ButtonControl_IsPressed(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<ButtonControl>() ? entity.GetComponent<ButtonControl>().PressedThisFrame
                                                          : false;
}

CH_SCRIPT_FUNC bool CheckboxControl_GetChecked(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<CheckboxControl>() ? entity.GetComponent<CheckboxControl>().Checked : false;
}

CH_SCRIPT_FUNC int ComboBoxControl_GetSelectedIndex(uint64_t entityID)
{
    Entity entity = GetEntity(entityID);
    return entity && entity.HasComponent<ComboBoxControl>() ? entity.GetComponent<ComboBoxControl>().SelectedIndex : 0;
}

CH_SCRIPT_FUNC Coral::String ComboBoxControl_GetItem(uint64_t entityID, int index)
{
    Entity entity = GetEntity(entityID);
    if (entity && entity.HasComponent<ComboBoxControl>())
    {
        auto& combo = entity.GetComponent<ComboBoxControl>();
        if (index >= 0 && index < (int)combo.Items.size())
        {
            return Coral::String::New(combo.Items[index]);
        }
    }
    return Coral::String::New("");
}

CH_SCRIPT_FUNC void UI_Text(Coral::String text)
{
    ImGui::Text("%s", ((std::string)text).c_str());
}

void RegisterUIGlue(Coral::ManagedAssembly& assembly) {
#define CH_ADD_INTERNAL_CALL(className, fieldName, funcPtr)                                                            \
    assembly.AddInternalCall("CHEngine." #className, #fieldName, (void*)funcPtr)

    CH_ADD_INTERNAL_CALL(UI, UI_Text_Ptr, UI_Text);
    CH_ADD_INTERNAL_CALL(ButtonControl, ButtonControl_IsPressed_Ptr, ButtonControl_IsPressed);
    CH_ADD_INTERNAL_CALL(CheckboxControl, CheckboxControl_GetChecked_Ptr, CheckboxControl_GetChecked);
    CH_ADD_INTERNAL_CALL(ComboBoxControl, ComboBoxControl_GetSelectedIndex_Ptr, ComboBoxControl_GetSelectedIndex);
    CH_ADD_INTERNAL_CALL(ComboBoxControl, ComboBoxControl_GetItem_Ptr, ComboBoxControl_GetItem);

#undef CH_ADD_INTERNAL_CALL
} } // namespace CHEngine

