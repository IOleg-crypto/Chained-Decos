#ifndef CH_UI_RENDER_WIDGETS_H
#define CH_UI_RENDER_WIDGETS_H

#include "engine/scene/components/control_component.h"
#include "engine/scene/entity.h"
#include "imgui.h"

namespace Chained
{

// Basic
bool RenderPanel(PanelData& panel, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderButton(const ButtonData& button, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle);
bool RenderLabel(const LabelData& label, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle);
bool RenderCheckbox(CheckboxData& cb, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle);
bool RenderProgressBar(const ProgressBarData& pb, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderSeparator(const SeparatorData& sep, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderRadioButton(RadioButtonData& radio, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle);

// Input
bool RenderSlider(SliderData& slider, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderComboBox(ComboBoxData& combo, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle);
bool RenderInputText(InputTextData& input, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderDragFloat(DragFloatData& drag, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderDragInt(DragIntData& drag, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderColorPicker(ColorPickerData& picker, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);

// Image
bool RenderImage(ImageData& img, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderImageButton(ImageButtonData& imgBtn, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle);

// Container
bool RenderTreeNode(TreeNodeData& node, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle);
bool RenderTabBar(TabBarData& bar, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderTabItem(TabItemData& item, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderCollapsingHeader(CollapsingHeaderData& header, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);
bool RenderVerticalLayoutGroup(const VerticalLayoutGroupData& layout, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);

// Plot
bool RenderPlot(const PlotData& plot, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size);

} // namespace Chained
#endif // CH_UI_RENDER_WIDGETS_H
