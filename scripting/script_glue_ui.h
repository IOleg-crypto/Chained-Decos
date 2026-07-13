#ifndef SCRIPT_GLUE_UI_H
#define SCRIPT_GLUE_UI_H
#include "script_glue_internal.h"
#include <variant>
#include <imgui_internal.h>

namespace Chained
{

// ── UI Controls ───────────────────────────────────────────────────────
CH_SCRIPT_FUNC bool ButtonControl_IsClicked(uint64_t entityID);
CH_SCRIPT_FUNC bool ButtonControl_IsDown(uint64_t entityID);
CH_SCRIPT_FUNC const Coral::UCChar* ButtonControl_GetLabel(uint64_t entityID);
CH_SCRIPT_FUNC void ButtonControl_SetLabel(uint64_t entityID, const Coral::UCChar* label);

CH_SCRIPT_FUNC bool CheckboxControl_GetChecked(uint64_t entityID);
CH_SCRIPT_FUNC void CheckboxControl_SetChecked(uint64_t entityID, bool checked);

CH_SCRIPT_FUNC const Coral::UCChar* LabelControl_GetText(uint64_t entityID);
CH_SCRIPT_FUNC void LabelControl_SetText(uint64_t entityID, const Coral::UCChar* text);

CH_SCRIPT_FUNC const Coral::UCChar* SliderControl_GetLabel(uint64_t entityID);
CH_SCRIPT_FUNC void SliderControl_SetLabel(uint64_t entityID, const Coral::UCChar* label);
CH_SCRIPT_FUNC float SliderControl_GetValue(uint64_t entityID);
CH_SCRIPT_FUNC void SliderControl_SetValue(uint64_t entityID, float value);
CH_SCRIPT_FUNC float SliderControl_GetMin(uint64_t entityID);
CH_SCRIPT_FUNC void SliderControl_SetMin(uint64_t entityID, float minVal);
CH_SCRIPT_FUNC float SliderControl_GetMax(uint64_t entityID);
CH_SCRIPT_FUNC void SliderControl_SetMax(uint64_t entityID, float maxVal);

CH_SCRIPT_FUNC float ProgressBarControl_GetProgress(uint64_t entityID);
CH_SCRIPT_FUNC void ProgressBarControl_SetProgress(uint64_t entityID, float progress);
CH_SCRIPT_FUNC const Coral::UCChar* ProgressBarControl_GetOverlayText(uint64_t entityID);
CH_SCRIPT_FUNC void ProgressBarControl_SetOverlayText(uint64_t entityID, const Coral::UCChar* text);
CH_SCRIPT_FUNC bool ProgressBarControl_GetShowPercentage(uint64_t entityID);
CH_SCRIPT_FUNC void ProgressBarControl_SetShowPercentage(uint64_t entityID, bool show);

CH_SCRIPT_FUNC bool   WidgetControl_GetActive(uint64_t entityID);
CH_SCRIPT_FUNC void   WidgetControl_SetActive(uint64_t entityID, bool active);
CH_SCRIPT_FUNC const Coral::UCChar* WidgetControl_GetTextColor(uint64_t entityID);
CH_SCRIPT_FUNC void   WidgetControl_SetTextColorRGBA(uint64_t entityID, int r, int g, int b, int a);

CH_SCRIPT_FUNC int ComboBoxControl_GetSelectedIndex(uint64_t entityID);

CH_SCRIPT_FUNC void ComboBoxControl_SetSelectedIndex(uint64_t entityID, int index);

CH_SCRIPT_FUNC void ComboBoxControl_AddItem(uint64_t entityID, const Coral::UCChar* item);

CH_SCRIPT_FUNC void ComboBoxControl_ClearItems(uint64_t entityID);

CH_SCRIPT_FUNC int ComboBoxControl_GetItemCount(uint64_t entityID);

CH_SCRIPT_FUNC const Coral::UCChar* ComboBoxControl_GetItem(uint64_t entityID, int index);

CH_SCRIPT_FUNC void UI_Text(const Coral::UCChar* text);

} // namespace Chained
#endif