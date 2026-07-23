#include "ui_control_renderer.h"
#include "ui_font_registry.h"
#include "ui_render_helpers.h"
#include "ui_render_widgets.h"

namespace Chained
{

bool RenderControl(UIFontRegistry& fontRegistry, Entity entity, UIControlComponent& control, const ImVec2& screenPos,
                   const ImVec2& size)
{
    if (std::holds_alternative<std::monostate>(control.Data))
    {
        return false;
    }

    const std::string* fontName = &control.TextStyle.FontName;

    ImFont* activeFont = nullptr;
    if (!fontName->empty() && *fontName != "Default")
    {
        activeFont = fontRegistry.GetFont(*fontName, control.TextStyle.FontSize);
    }

    // Fall back to the project's default font, not the editor's ImGui font.
    if (!activeFont)
    {
        activeFont = fontRegistry.GetDefaultFont();
    }

    bool changed = false;

    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ButtonData>)
            {
                changed = RenderButton(arg, control, screenPos, size, activeFont, control.TextStyle);
            }
            else if constexpr (std::is_same_v<T, PanelData>)
            {
                changed = RenderPanel(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, LabelData>)
            {
                changed = RenderLabel(arg, control, screenPos, size, activeFont, control.TextStyle);
            }
            else if constexpr (std::is_same_v<T, CheckboxData>)
            {
                changed = RenderCheckbox(arg, control, screenPos, size, activeFont, control.TextStyle);
            }
            else if constexpr (std::is_same_v<T, SliderData>)
            {
                changed = RenderSlider(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, ProgressBarData>)
            {
                changed = RenderProgressBar(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, ImageData>)
            {
                changed = RenderImage(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, ImageButtonData>)
            {
                changed = RenderImageButton(arg, control, screenPos, size, activeFont, control.TextStyle);
            }
            else if constexpr (std::is_same_v<T, ComboBoxData>)
            {
                changed = RenderComboBox(arg, control, screenPos, size, activeFont, control.TextStyle);
            }
            else if constexpr (std::is_same_v<T, InputTextData>)
            {
                changed = RenderInputText(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, SeparatorData>)
            {
                changed = RenderSeparator(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, RadioButtonData>)
            {
                changed = RenderRadioButton(arg, control, screenPos, size, activeFont, control.TextStyle);
            }
            else if constexpr (std::is_same_v<T, ColorPickerData>)
            {
                changed = RenderColorPicker(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, DragFloatData>)
            {
                changed = RenderDragFloat(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, DragIntData>)
            {
                changed = RenderDragInt(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, TreeNodeData>)
            {
                changed = RenderTreeNode(arg, control, screenPos, size, activeFont, control.TextStyle);
            }
            else if constexpr (std::is_same_v<T, TabBarData>)
            {
                changed = RenderTabBar(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, TabItemData>)
            {
                changed = RenderTabItem(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, CollapsingHeaderData>)
            {
                changed = RenderCollapsingHeader(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, PlotData>)
            {
                changed = RenderPlot(arg, control, screenPos, size);
            }
            else if constexpr (std::is_same_v<T, VerticalLayoutGroupData>)
            {
                changed = RenderVerticalLayoutGroup(arg, control, screenPos, size);
            }
        },
        control.Data);

    control.ValueChanged = changed;
    return changed;
}

} // namespace Chained
