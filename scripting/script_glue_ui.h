#ifndef SCRIPT_GLUE_UI_H
#define SCRIPT_GLUE_UI_H
#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include <variant>
#include <imgui_internal.h>

namespace Chained
{

void RegisterGlueUI();

// ── UI Controls ───────────────────────────────────────────────────────
CH_SCRIPT_FUNC bool ButtonControl_IsClicked(uint64_t entityID);

CH_SCRIPT_FUNC bool ButtonControl_IsDown(uint64_t entityID);

CH_SCRIPT_FUNC bool CheckboxControl_GetChecked(uint64_t entityID);

CH_SCRIPT_FUNC int ComboBoxControl_GetSelectedIndex(uint64_t entityID);

CH_SCRIPT_FUNC void ComboBoxControl_SetSelectedIndex(uint64_t entityID, int index);

CH_SCRIPT_FUNC void ComboBoxControl_AddItem(uint64_t entityID, const char16_t* item);

CH_SCRIPT_FUNC void ComboBoxControl_ClearItems(uint64_t entityID);

CH_SCRIPT_FUNC int ComboBoxControl_GetItemCount(uint64_t entityID);

CH_SCRIPT_FUNC const char16_t* ComboBoxControl_GetItem(uint64_t entityID, int index);

CH_SCRIPT_FUNC void UI_Text(const char16_t* text);

} // namespace Chained
#endif