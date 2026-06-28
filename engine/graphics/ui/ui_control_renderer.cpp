#include "ui_control_renderer.h"
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

struct StyleCounts
{
    int colors = 0;
    int vars = 0;
    int fonts = 0;
    bool disabled = false;
};

inline ImVec4 ToImVec4(const Color& c)
{
    return ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}

static StyleCounts PushUIStyle(const UIStyle& style, bool interactable)
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
// Explicit internal control rendering functions
// ---------------------------------------------------------------------------

static bool RenderPanel(PanelData& panel, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImU32 bgColor    = ImGui::GetColorU32(ToImVec4(wc.BoxStyle.BackgroundColor));
    ImU32 borderCol  = ImGui::GetColorU32(ToImVec4(wc.BoxStyle.BorderColor));
    ImVec2 pMax      = {pos.x + size.x, pos.y + size.y};

    AssetHandle textureHandle = (AssetHandle)panel.TextureHandle;
    
    if (textureHandle != 0)
    {
        auto textureAsset = ServiceLocator::Get<AssetManager>()->Get<TextureAsset>(textureHandle);
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
    return false;
}

static bool RenderLabel(const LabelData& label, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::Button(label.Text.c_str(), size);
    return false;
}

static bool RenderButton(const ButtonData& button, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::Button(button.Label.c_str(), size);
    return wc.PressedThisFrame;
}

static bool RenderSlider(SliderData& slider, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    return ImGui::SliderFloat("##slider", &slider.Value, slider.Min, slider.Max);
}

static bool RenderCheckbox(CheckboxData& cb, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    return ImGui::Checkbox(cb.Label.c_str(), &cb.Checked);
}

static bool RenderImage(const ImageData& image, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    auto textureAsset = ServiceLocator::Get<AssetManager>()->Get<TextureAsset>((AssetHandle)image.TextureHandle);
    if (textureAsset && textureAsset->GetState() == AssetState::Ready)
    {
        ImGui::Image((ImTextureID)(uintptr_t)textureAsset->GetTexture()->GetRendererID(), size);
    }
    return false;
}

static bool RenderInputText(InputTextData& it, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
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

static bool RenderProgressBar(const ProgressBarData& pb, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::ProgressBar(pb.Progress, size);
    return false;
}

static bool RenderComboBox(ComboBoxData& cb, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
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

static bool RenderImageButton(const ImageButtonData& ib, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    auto textureAsset = ServiceLocator::Get<AssetManager>()->Get<TextureAsset>((AssetHandle)ib.TextureHandle);
    if (textureAsset && textureAsset->GetState() == AssetState::Ready)
    {
        ImGui::ImageButton("##ib", (ImTextureID)(uintptr_t)textureAsset->GetTexture()->GetRendererID(), size);
    }
    return wc.PressedThisFrame;
}

static bool RenderRadioButton(RadioButtonData& rb, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
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

static bool RenderColorPicker(ColorPickerData& cp, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
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

static bool RenderSeparator(const SeparatorData& sep, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::Separator();
    return false;
}

static bool RenderDragFloat(DragFloatData& df, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    return ImGui::DragFloat(df.Label.c_str(), &df.Value, df.Speed, df.Min, df.Max);
}

static bool RenderDragInt(DragIntData& di, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    return ImGui::DragInt(di.Label.c_str(), &di.Value, di.Speed, di.Min, di.Max);
}

static bool RenderTreeNode(TreeNodeData& tn, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGuiTreeNodeFlags flags = 0;
    if (tn.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (tn.IsLeaf) flags |= ImGuiTreeNodeFlags_Leaf;
    
    bool open = ImGui::TreeNodeEx(tn.Label.c_str(), flags);
    tn.IsOpen = open;
    return false;
}

static bool RenderCollapsingHeader(CollapsingHeaderData& ch, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGuiTreeNodeFlags flags = 0;
    if (ch.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    
    bool open = ImGui::CollapsingHeader(ch.Label.c_str(), flags);
    ch.IsOpen = open;
    return false;
}

static bool RenderPlotLines(const PlotLinesData& pl, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::PlotLines(pl.Label.c_str(), pl.Values.data(), (int)pl.Values.size(), 0, pl.OverlayText.c_str(), pl.ScaleMin, pl.ScaleMax, ImVec2(pl.GraphSize.x, pl.GraphSize.y));
    return false;
}

static bool RenderPlotHistogram(const PlotHistogramData& ph, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::PlotHistogram(ph.Label.c_str(), ph.Values.data(), (int)ph.Values.size(), 0, ph.OverlayText.c_str(), ph.ScaleMin, ph.ScaleMax, ImVec2(ph.GraphSize.x, ph.GraphSize.y));
    return false;
}

static bool RenderTabBar(TabBarData& tb, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGuiTabBarFlags flags = 0;
    if (tb.Reorderable) flags |= ImGuiTabBarFlags_Reorderable;
    if (tb.AutoSelectNewTabs) flags |= ImGuiTabBarFlags_AutoSelectNewTabs;

    if (!ImGui::BeginTabBar(tb.Label.c_str(), flags)) return false;
    ImGui::EndTabBar();
    return false;
}

static bool RenderVerticalLayoutGroup(const VerticalLayoutGroupData& vlg, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    return false; 
}

static bool RenderTabItem(const TabItemData& ti, Entity entity, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    return false;  
}

// ---------------------------------------------------------------------------
// Main Control Dispatcher
// ---------------------------------------------------------------------------

bool RenderControl(const UIFontRegistry& fontRegistry,
                   Entity entity, UIControlComponent& control, const ImVec2& screenPos, const ImVec2& size)
{
    StyleCounts styleState = PushUIStyle(control.BoxStyle, true);
    PushTextStyle(fontRegistry, control.TextStyle, styleState);

    bool changed = false;

    // A simple, elegant pattern without overloading ambiguities
    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ButtonData>)            changed = RenderButton(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, PanelData>)        changed = RenderPanel(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, LabelData>)        changed = RenderLabel(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, SliderData>)       changed = RenderSlider(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, CheckboxData>)     changed = RenderCheckbox(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, ImageData>)        changed = RenderImage(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, InputTextData>)    changed = RenderInputText(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, ProgressBarData>)  changed = RenderProgressBar(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, ComboBoxData>)     changed = RenderComboBox(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, ImageButtonData>)  changed = RenderImageButton(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, RadioButtonData>)  changed = RenderRadioButton(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, ColorPickerData>)  changed = RenderColorPicker(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, SeparatorData>)    changed = RenderSeparator(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, DragFloatData>)    changed = RenderDragFloat(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, DragIntData>)      changed = RenderDragInt(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, TreeNodeData>)     changed = RenderTreeNode(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, CollapsingHeaderData>) changed = RenderCollapsingHeader(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, PlotLinesData>)    changed = RenderPlotLines(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, PlotHistogramData>) changed = RenderPlotHistogram(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, TabBarData>)       changed = RenderTabBar(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, VerticalLayoutGroupData>) changed = RenderVerticalLayoutGroup(arg, entity, control, screenPos, size);
        else if constexpr (std::is_same_v<T, TabItemData>)      changed = RenderTabItem(arg, entity, control, screenPos, size);
    }, control.Data);

    control.ValueChanged = changed;
    PopUIStyle(styleState);
    return true;
}

} // namespace Chained
