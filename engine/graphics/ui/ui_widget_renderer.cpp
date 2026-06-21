#include "ui_widget_renderer.h"
#include "ui_renderer.h"
#include "ui_font_registry.h"

#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/assets/types/texture_asset.h"

namespace Chained
{

// ---------------------------------------------------------------------------
// Style helpers
// ---------------------------------------------------------------------------

StyleCounts PushUIStyle(const UIStyle& style, bool interactable)
{
    StyleCounts stylecount;

    ImGui::PushStyleColor(ImGuiCol_Button,         ToImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ToImVec4(style.HoverColor));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ToImVec4(style.PressedColor));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,        ToImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_Border,         ToImVec4(style.BorderColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ToImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ToImVec4(style.HoverColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ToImVec4(style.PressedColor));
    stylecount.colors += 8;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   style.Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,   style.Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.BorderSize);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,    ImVec2(style.Padding, style.Padding));
    stylecount.vars += 4;

    if (!interactable)
    {
        ImGui::BeginDisabled(true);
        stylecount.disabled = true;
    }

    return stylecount;
}

void PushTextStyle(const UIFontRegistry& fontRegistry, const TextStyle& text, StyleCounts& c)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(text.TextColor));
    c.colors++;

    float hAlign = (text.Horizontal == HorizontalAlignment::Left)   ? 0.0f
                 : (text.Horizontal == HorizontalAlignment::Center) ? 0.5f : 1.0f;
    float vAlign = (text.Vertical == VerticalAlignment::Top)       ? 0.0f
                 : (text.Vertical == VerticalAlignment::Center)    ? 0.5f : 1.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(hAlign, vAlign));
    c.vars++;

    const std::string& fontName = text.FontName;

    if (!fontName.empty() && fontName != "Default")
    {
        ImFont* font = fontRegistry.GetFont(fontName, (int)text.FontSize);
        if (font)
        {
            ImGui::PushFont(font);
            c.fonts++;
        }
    }
}

void PopUIStyle(const StyleCounts& c)
{
    for (int i = 0; i < c.fonts; ++i) {
        ImGui::PopFont();
    }
    ImGui::PopStyleVar(c.vars);
    ImGui::PopStyleColor(c.colors);
    if (c.disabled) {
        ImGui::EndDisabled();
    }
}

// ---------------------------------------------------------------------------
// Widget rendering
// ---------------------------------------------------------------------------

void RenderPanel(PanelData& panel, WidgetComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImU32 bgColor    = ImGui::GetColorU32(ToImVec4(wc.BoxStyle.BackgroundColor));
    ImU32 borderCol  = ImGui::GetColorU32(ToImVec4(wc.BoxStyle.BorderColor));
    ImVec2 pMax      = {pos.x + size.x, pos.y + size.y};

    AssetHandle textureHandle = (AssetHandle)panel.TextureHandle;
    
    if (textureHandle != 0)
    {
        auto textureAsset = ServiceLocator::Get<AssetManager>()->GetAsset<TextureAsset>(textureHandle);
        if (textureAsset && textureAsset->GetState() == AssetState::Ready)
        {
            auto texture = textureAsset->GetTexture();
            ImTextureID texId = (ImTextureID)(uintptr_t)texture->GetRendererID();
            dl->AddImageRounded(texId, pos, pMax, {0,1}, {1,0}, IM_COL32_WHITE, wc.BoxStyle.Rounding);
        }
    }
    else
    {
        dl->AddRectFilled(pos, pMax, bgColor, wc.BoxStyle.Rounding);
    }

    if (wc.BoxStyle.BorderSize > 0.0f)
    {
        dl->AddRect(pos, pMax, borderCol, wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);
    }
}

void RenderLabel(const LabelData& label, const WidgetComponent& wc, const ImVec2& size)
{
    ImGui::Button(label.Text.c_str(), size);
}

bool RenderButton(Entity entity, const ButtonData& button, const WidgetComponent& wc, const ImVec2& size)
{
    ImGui::Button(button.Label.c_str(), size);
    return wc.PressedThisFrame;
}

bool RenderSlider(SliderData& slider, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    return ImGui::SliderFloat("##slider", &slider.Value, slider.Min, slider.Max);
}

bool RenderCheckbox(CheckboxData& cb, WidgetComponent& wc)
{
    return ImGui::Checkbox(cb.Label.c_str(), &cb.Checked);
}

void RenderImage(const ImageData& image, const WidgetComponent& wc, const ImVec2& size)
{
    auto textureAsset = ServiceLocator::Get<AssetManager>()->GetAsset<TextureAsset>((AssetHandle)image.TextureHandle);
    if (textureAsset && textureAsset->GetState() == AssetState::Ready)
    {
        ImGui::Image((ImTextureID)(uintptr_t)textureAsset->GetTexture()->GetRendererID(), size);
    }
}

bool RenderInputText(Entity entity, InputTextData& it, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    char buffer[1024];
    strncpy(buffer, it.Text.c_str(), sizeof(buffer));
    if (ImGui::InputText("##input", buffer, sizeof(buffer)))
    {
        it.Text = buffer;
        return true;
    }
    return false;
}

void RenderProgressBar(const ProgressBarData& pb, const WidgetComponent& wc, const ImVec2& size)
{
    ImGui::ProgressBar(pb.Progress, size);
}

bool RenderComboBox(ComboBoxData& cb, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    bool changed = false;
    if (ImGui::BeginCombo(cb.Label.c_str(), cb.Items[cb.SelectedIndex].c_str()))
    {
        for (int i = 0; i < (int)cb.Items.size(); i++)
        {
            if (ImGui::Selectable(cb.Items[i].c_str(), i == cb.SelectedIndex))
            {
                cb.SelectedIndex = i;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

bool RenderImageButton(const ImageButtonData& ib, const WidgetComponent& wc, const ImVec2& size)
{
    auto textureAsset = ServiceLocator::Get<AssetManager>()->GetAsset<TextureAsset>((AssetHandle)ib.TextureHandle);
    if (textureAsset && textureAsset->GetState() == AssetState::Ready)
    {
        ImGui::ImageButton("##ib", (ImTextureID)(uintptr_t)textureAsset->GetTexture()->GetRendererID(), size);
    }
    return wc.PressedThisFrame;
}

bool RenderRadioButton(RadioButtonData& rb, WidgetComponent& wc)
{
    bool changed = false;
    for (int i = 0; i < (int)rb.Options.size(); i++)
    {
        if (ImGui::RadioButton(rb.Options[i].c_str(), rb.SelectedIndex == i))
        {
            rb.SelectedIndex = i;
            changed = true;
        }
        if (rb.Horizontal && i < (int)rb.Options.size() - 1) ImGui::SameLine();
    }
    return changed;
}

bool RenderColorPicker(ColorPickerData& cp, WidgetComponent& wc)
{
    float col[4] = {cp.SelectedColor.r / 255.f, cp.SelectedColor.g / 255.f, cp.SelectedColor.b / 255.f, cp.SelectedColor.a / 255.f};
    bool changed = false;
    if (cp.ShowAlpha) changed = ImGui::ColorEdit4(cp.Label.c_str(), col);
    else changed = ImGui::ColorEdit3(cp.Label.c_str(), col);
    
    if (changed)
    {
        cp.SelectedColor = {(uint8_t)(col[0] * 255), (uint8_t)(col[1] * 255), (uint8_t)(col[2] * 255), (uint8_t)(col[3] * 255)};
    }
    return changed;
}

void RenderSeparator(const SeparatorData& sep)
{
    ImGui::Separator();
}

bool RenderDragFloat(DragFloatData& df, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    return ImGui::DragFloat(df.Label.c_str(), &df.Value, df.Speed, df.Min, df.Max);
}

bool RenderDragInt(DragIntData& di, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    return ImGui::DragInt(di.Label.c_str(), &di.Value, di.Speed, di.Min, di.Max);
}

bool RenderTreeNode(TreeNodeData& tn, WidgetComponent& wc)
{
    ImGuiTreeNodeFlags flags = 0;
    if (tn.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (tn.IsLeaf) flags |= ImGuiTreeNodeFlags_Leaf;
    
    bool open = ImGui::TreeNodeEx(tn.Label.c_str(), flags);
    tn.IsOpen = open;
    return open;
}

bool RenderCollapsingHeader(CollapsingHeaderData& ch, WidgetComponent& wc)
{
    ImGuiTreeNodeFlags flags = 0;
    if (ch.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    
    bool open = ImGui::CollapsingHeader(ch.Label.c_str(), flags);
    ch.IsOpen = open;
    return open;
}

bool RenderPlotLines(const PlotLinesData& pl, const WidgetComponent& wc)
{
    ImGui::PlotLines(pl.Label.c_str(), pl.Values.data(), (int)pl.Values.size(), 0, pl.OverlayText.c_str(), pl.ScaleMin, pl.ScaleMax, ImVec2(pl.GraphSize.x, pl.GraphSize.y));
    return false;
}

bool PlotHistogram(const PlotHistogramData& ph, const WidgetComponent& wc)
{
    ImGui::PlotHistogram(ph.Label.c_str(), ph.Values.data(), (int)ph.Values.size(), 0, ph.OverlayText.c_str(), ph.ScaleMin, ph.ScaleMax, ImVec2(ph.GraphSize.x, ph.GraphSize.y));
    return false;
}

void RenderTabBar(Entity tabBarEntity, TabBarData& tb, WidgetComponent& wc, entt::registry& registry)
{
    ImGuiTabBarFlags flags = 0;
    if (tb.Reorderable) flags |= ImGuiTabBarFlags_Reorderable;
    if (tb.AutoSelectNewTabs) flags |= ImGuiTabBarFlags_AutoSelectNewTabs;

    if (!ImGui::BeginTabBar(tb.Label.c_str(), flags)) return;
    ImGui::EndTabBar();
}

bool Dispatcher::Render(const UIFontRegistry& fontRegistry,
                        Entity entity, WidgetComponent& widget, const ImVec2& screenPos, const ImVec2& size)
{
    StyleCounts styleState = PushUIStyle(widget.BoxStyle);
    PushTextStyle(fontRegistry, widget.TextStyle, styleState);

    bool handled = false;
    bool changed = false;

    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ButtonData>)       handled = RenderButton(entity, arg, widget, size);
            else if constexpr (std::is_same_v<T, PanelData>)   RenderPanel(arg, widget, screenPos, size);
            else if constexpr (std::is_same_v<T, LabelData>)   RenderLabel(arg, widget, size);
            else if constexpr (std::is_same_v<T, SliderData>)  changed = RenderSlider(arg, widget, size);
            else if constexpr (std::is_same_v<T, CheckboxData>) changed = RenderCheckbox(arg, widget);
            else if constexpr (std::is_same_v<T, ImageData>)    RenderImage( arg, widget, size);
            else if constexpr (std::is_same_v<T, InputTextData>) changed = RenderInputText(entity, arg, widget, size);
            else if constexpr (std::is_same_v<T, ProgressBarData>) RenderProgressBar(arg, widget, size);
            else if constexpr (std::is_same_v<T, ComboBoxData>) changed = RenderComboBox(arg, widget, size);
            else if constexpr (std::is_same_v<T, ImageButtonData>) changed = RenderImageButton(arg, widget, size);
            else if constexpr (std::is_same_v<T, RadioButtonData>) changed = RenderRadioButton(arg, widget);
            else if constexpr (std::is_same_v<T, ColorPickerData>) changed = RenderColorPicker(arg, widget);
            else if constexpr (std::is_same_v<T, SeparatorData>)   RenderSeparator(arg);
            else if constexpr (std::is_same_v<T, DragFloatData>)   changed = RenderDragFloat(arg, widget, size);
            else if constexpr (std::is_same_v<T, DragIntData>)     changed = RenderDragInt(arg, widget, size);
            else if constexpr (std::is_same_v<T, TreeNodeData>)    handled = RenderTreeNode(arg, widget);
            else if constexpr (std::is_same_v<T, CollapsingHeaderData>) handled = RenderCollapsingHeader(arg, widget);
            else if constexpr (std::is_same_v<T, TabBarData>)      RenderTabBar(entity, arg, widget, entity.GetRegistry());
            else if constexpr (std::is_same_v<T, PlotLinesData>)   RenderPlotLines(arg, widget);
            else if constexpr (std::is_same_v<T, PlotHistogramData>) PlotHistogram(arg, widget);
        },
        widget.Data);

    widget.ValueChanged = changed;
    PopUIStyle(styleState);
    return true;
}

} // namespace Chained

