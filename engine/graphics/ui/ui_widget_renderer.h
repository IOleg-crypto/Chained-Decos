#ifndef CH_UI_WIDGET_RENDERER_H
#define CH_UI_WIDGET_RENDERER_H

#include "engine/scene/components.h"
#include "engine/scene/entity.h"
#include "imgui.h"
#include "entt/entt.hpp"

namespace Chained
{
class UIFontRegistry;
class AssetManager;

// Converts engine Color to ImVec4
inline ImVec4 ToImVec4(const Color& color)
{
    return ImVec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
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
void        PushTextStyle(const UIFontRegistry& fontRegistry, const TextStyle& text, StyleCounts& c);
void        PopUIStyle(const StyleCounts& c);


void RenderPanel   (AssetManager* assetManager, PanelData& panel, WidgetComponent& wc, const ImVec2& pos, const ImVec2& size);
void RenderLabel   (const LabelData& label, const WidgetComponent& wc, const ImVec2& size);
bool RenderButton  (Entity entity, const ButtonData& button, const WidgetComponent& wc, const ImVec2& size);
bool RenderSlider  (SliderData& slider, WidgetComponent& wc, const ImVec2& size);
bool RenderCheckbox(CheckboxData& cb, WidgetComponent& wc);
void RenderImage   (AssetManager* assetManager, const ImageData& image, const WidgetComponent& wc, const ImVec2& size);
bool RenderInputText(Entity entity, InputTextData& it, WidgetComponent& wc, const ImVec2& size);
void RenderProgressBar(const ProgressBarData& pb, const WidgetComponent& wc, const ImVec2& size);
bool RenderComboBox(ComboBoxData& cb, WidgetComponent& wc, const ImVec2& size);
bool RenderImageButton(AssetManager* assetManager, const ImageButtonData& ib, const WidgetComponent& wc, const ImVec2& size);
bool RenderRadioButton(RadioButtonData& rb, WidgetComponent& wc);
bool RenderColorPicker(ColorPickerData& cp, WidgetComponent& wc);
void RenderSeparator(const SeparatorData& sep);
bool RenderDragFloat(DragFloatData& df, WidgetComponent& wc, const ImVec2& size);
bool RenderDragInt  (DragIntData& di, WidgetComponent& wc, const ImVec2& size);
bool RenderTreeNode(TreeNodeData& tn, WidgetComponent& wc);
bool RenderCollapsingHeader(CollapsingHeaderData& ch, WidgetComponent& wc);
bool RenderPlotLines(const PlotLinesData& pl, const WidgetComponent& wc);
bool PlotHistogram(const PlotHistogramData& ph, const WidgetComponent& wc);
void RenderTabBar  (Entity tabBarEntity, TabBarData& tb, WidgetComponent& wc, entt::registry& registry);

// Centralized dispatcher that handles styling and rendering for any widget type
struct Dispatcher
{
    static bool Render(const UIFontRegistry& fontRegistry,
                       Entity entity, WidgetComponent& widget, const ImVec2& screenPos, const ImVec2& size);
};

} // namespace Chained


#endif // CH_UI_WIDGET_RENDERER_H
