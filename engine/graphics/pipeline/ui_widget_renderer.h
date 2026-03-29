#ifndef CH_UI_WIDGET_RENDERER_H
#define CH_UI_WIDGET_RENDERER_H

#include "engine/scene/components.h"
#include "engine/scene/entity.h"
#include "imgui.h"
#include "entt/entt.hpp"

namespace CHEngine::UI
{

// Converts engine Color to ImVec4
inline ImVec4 ToImVec4(const Color& c)
{
    return ImVec4(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
}

// Style push/pop helpers
struct StyleCounts
{
    int  colors   = 0;
    int  vars     = 0;
    int  fonts    = 0;
    bool disabled = false;
};

StyleCounts PushUIStyle(const UIStyle& style, bool interactable = true);
void        PushTextStyle(const TextStyle& text, StyleCounts& c);
void        PopUIStyle(const StyleCounts& c);

// --- Widget rendering functions ---

void RenderPanel   (const PanelControl& panel, const ImVec2& pos, const ImVec2& size);
void RenderLabel   (const LabelControl& label, const ImVec2& size);
bool RenderButton  (Entity entity, ButtonControl& button, const ImVec2& size);
bool RenderSlider  (SliderControl& slider, const ImVec2& size);
bool RenderCheckbox(CheckboxControl& cb);
void RenderImage   (ImageControl& image, const ImVec2& size);
bool RenderInputText(Entity entity, InputTextControl& it, const ImVec2& size);
void RenderProgressBar(const ProgressBarControl& pb, const ImVec2& size);
bool RenderComboBox(ComboBoxControl& cb, const ImVec2& size);
bool RenderImageButton(ImageButtonControl& ib, const ImVec2& size);
bool RenderRadioButton(RadioButtonControl& rb);
bool RenderColorPicker(ColorPickerControl& cp);
void RenderSeparator(const SeparatorControl& sep);
bool RenderDragFloat(DragFloatControl& df, const ImVec2& size);
bool RenderDragInt  (DragIntControl& di, const ImVec2& size);
bool RenderTreeNode(TreeNodeControl& tn);
bool RenderCollapsingHeader(CollapsingHeaderControl& ch);
bool RenderPlotLines(const PlotLinesControl& pl);
bool RenderPlotHistogram(const PlotHistogramControl& ph);
void RenderTabBar  (Entity tabBarEntity, const TabBarControl& tb, entt::registry& registry);

} // namespace CHEngine::UI

#endif // CH_UI_WIDGET_RENDERER_H
