#include "ui_widget_renderer.h"
#include "ui_renderer.h"
#include "engine/graphics/texture_system.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include <algorithm>

namespace CHEngine::UI
{

// ---------------------------------------------------------------------------
// Style helpers
// ---------------------------------------------------------------------------

StyleCounts PushUIStyle(const UIStyle& style, bool interactable)
{
    StyleCounts c;

    ImGui::PushStyleColor(ImGuiCol_Button,         ToImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ToImVec4(style.HoverColor));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ToImVec4(style.PressedColor));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,        ToImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_Border,         ToImVec4(style.BorderColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ToImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ToImVec4(style.HoverColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ToImVec4(style.PressedColor));
    c.colors += 8;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   style.Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,   style.Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.BorderSize);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,    ImVec2(style.Padding, style.Padding));
    c.vars += 4;

    if (!interactable)
    {
        ImGui::BeginDisabled(true);
        c.disabled = true;
    }

    return c;
}

void PushTextStyle(const TextStyle& text, StyleCounts& c)
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

    // Default/empty uses regular ImGui font to avoid runtime font atlas mutations.
    if (!fontName.empty() && fontName != "Default")
    {
        ImFont* font = ServiceLocator::Get<UIRenderer>().GetFontRegistry().GetFont(fontName, text.FontSize);
        if (font)
        {
            ImGui::PushFont(font);
            c.fonts++;
        }
    }
}

void PopUIStyle(const StyleCounts& c)
{
    for (int i = 0; i < c.fonts; ++i) ImGui::PopFont();
    ImGui::PopStyleVar(c.vars);
    ImGui::PopStyleColor(c.colors);
    if (c.disabled) ImGui::EndDisabled();
}

// ---------------------------------------------------------------------------
// Widget rendering
// ---------------------------------------------------------------------------

void RenderPanel(const PanelData& panel, WidgetComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImU32 bgColor    = ImGui::GetColorU32(ToImVec4(wc.BoxStyle.BackgroundColor));
    ImU32 borderCol  = ImGui::GetColorU32(ToImVec4(wc.BoxStyle.BorderColor));
    ImVec2 pMax      = {pos.x + size.x, pos.y + size.y};

    TextureHandle textureHandle = panel.TextureHandle;
    if (textureHandle == 0 && !panel.TexturePath.empty())
    {
        textureHandle = ServiceLocator::Get<TextureSystem>().LoadTexture(panel.TexturePath);
    }

    if (textureHandle != 0)
    {
        auto texture = ServiceLocator::Get<TextureSystem>().GetTexture(textureHandle);
        if (texture && texture->IsReady())
        {
            ImTextureID texId = (ImTextureID)(uintptr_t)texture->GetRendererID();
            dl->AddImageRounded(texId, pos, pMax, {0,1}, {1,0}, IM_COL32_WHITE, wc.BoxStyle.Rounding);
        }
    }
    else if (wc.BoxStyle.UseGradient)
    {
        ImU32 gradColor = ImGui::GetColorU32(ToImVec4(wc.BoxStyle.GradientColor));
        dl->AddRectFilledMultiColor(pos, pMax, bgColor, bgColor, gradColor, gradColor);
    }
    else
    {
        dl->AddRectFilled(pos, pMax, bgColor, wc.BoxStyle.Rounding);
    }

    if (wc.BoxStyle.BorderSize > 0.0f)
        dl->AddRect(pos, pMax, borderCol, wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);
}

void RenderLabel(const LabelData& label, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + size.x);
    ImVec2 ts = ImGui::CalcTextSize(label.Text.c_str(), nullptr, true, size.x);

    float sx = 0, sy = 0;
    if (wc.TextStyle.Horizontal == HorizontalAlignment::Center)  sx = (size.x - ts.x) * 0.5f;
    else if (wc.TextStyle.Horizontal == HorizontalAlignment::Right) sx = size.x - ts.x;
    if (wc.TextStyle.Vertical == VerticalAlignment::Center)    sy = (size.y - ts.y) * 0.5f;
    else if (wc.TextStyle.Vertical == VerticalAlignment::Bottom)  sy = size.y - ts.y;

    ImGui::SetCursorPos({ImGui::GetCursorPosX() + sx, ImGui::GetCursorPosY() + sy});
    ImGui::TextUnformatted(label.Text.c_str());
    ImGui::PopTextWrapPos();
}

bool RenderButton(Entity entity, ButtonData& button, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::PushID((int)entity);
    ImGui::InvisibleButton(button.Label.c_str(), size);

    wc.IsHovered = ImGui::IsItemHovered();
    wc.IsDown    = ImGui::IsItemActive();
    if (ImGui::IsItemClicked())
    {
        CH_CORE_INFO("UI: Button '{}' clicked", button.Label);
        wc.PressedThisFrame = true;
    }

    bool handled = ImGui::IsItemActive();

    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImVec2 pos       = ImGui::GetItemRectMin();
    ImVec2 aSize     = size;

    if (wc.BoxStyle.State.CurrentScale != 1.0f)
    {
        ImVec2 center = {pos.x + size.x * 0.5f, pos.y + size.y * 0.5f};
        aSize.x *= wc.BoxStyle.State.CurrentScale;
        aSize.y *= wc.BoxStyle.State.CurrentScale;
        pos.x = center.x - aSize.x * 0.5f;
        pos.y = center.y - aSize.y * 0.5f;
    }

    ImVec2 pMax = {pos.x + aSize.x, pos.y + aSize.y};
    ImU32 color = ImGui::GetColorU32(ToImVec4(wc.BoxStyle.State.CurrentColor));
    dl->AddRectFilled(pos, pMax, color, wc.BoxStyle.Rounding);

    if (wc.BoxStyle.BorderSize > 0.0f)
        dl->AddRect(pos, pMax, ImGui::GetColorU32(ToImVec4(wc.BoxStyle.BorderColor)), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);

    ImVec2 ts  = ImGui::CalcTextSize(button.Label.c_str());
    ImVec2 tp  = {pos.x + (aSize.x - ts.x) * 0.5f, pos.y + (aSize.y - ts.y) * 0.5f};
    dl->AddText(tp, ImGui::GetColorU32(ImGuiCol_Text), button.Label.c_str());

    ImGui::PopID();
    return handled;
}

bool RenderSlider(SliderData& slider, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    wc.ValueChanged = ImGui::SliderFloat(slider.Label.c_str(), &slider.Value, slider.Min, slider.Max);
    return ImGui::IsItemActive();
}

bool RenderCheckbox(CheckboxData& cb, WidgetComponent& wc)
{
    wc.ValueChanged = ImGui::Checkbox(cb.Label.c_str(), &cb.Checked);
    return ImGui::IsItemActive();
}

void RenderImage(ImageData& image, WidgetComponent& wc, const ImVec2& size)
{
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImVec2 pos      = ImGui::GetCursorScreenPos();
    ImVec2 aSize    = size;

    ImGui::InvisibleButton("##img", size);
    wc.IsHovered = ImGui::IsItemHovered();
    wc.IsDown    = ImGui::IsItemActive();

    if (wc.BoxStyle.State.CurrentScale != 1.0f)
    {
        ImVec2 center = {pos.x + size.x * 0.5f, pos.y + size.y * 0.5f};
        aSize.x *= wc.BoxStyle.State.CurrentScale;
        aSize.y *= wc.BoxStyle.State.CurrentScale;
        pos.x = center.x - aSize.x * 0.5f;
        pos.y = center.y - aSize.y * 0.5f;
    }

    ImVec2 pMax = {pos.x + aSize.x, pos.y + aSize.y};

    TextureHandle textureHandle = image.TextureHandle;
    if (textureHandle == 0 && !image.TexturePath.empty())
    {
        textureHandle = ServiceLocator::Get<TextureSystem>().LoadTexture(image.TexturePath);
        image.TextureHandle = textureHandle;
    }

    if (textureHandle != 0)
    {
        auto tex = ServiceLocator::Get<TextureSystem>().GetTexture(textureHandle);
        if (tex && tex->IsReady())
        {
            ImTextureID tid = (ImTextureID)(uintptr_t)tex->GetRendererID();
            dl->AddImageRounded(tid, pos, pMax, {0,1}, {1,0}, ImGui::GetColorU32(ToImVec4(image.TintColor)), wc.BoxStyle.Rounding);
        }
    }
    else
    {
        ImU32 bg = ImGui::GetColorU32(ToImVec4(wc.BoxStyle.State.CurrentColor));
        if (wc.BoxStyle.UseGradient)
        {
            ImU32 gc = ImGui::GetColorU32(ToImVec4(wc.BoxStyle.GradientColor));
            dl->AddRectFilledMultiColor(pos, pMax, bg, bg, gc, gc);
        }
        else
        {
            dl->AddRectFilled(pos, pMax, bg, wc.BoxStyle.Rounding);
        }
    }

    if (wc.BoxStyle.BorderSize > 0.0f)
        dl->AddRect(pos, pMax, ImGui::GetColorU32(ToImVec4(image.BorderColor)), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);
}

bool RenderInputText(Entity entity, InputTextData& it, WidgetComponent& wc, const ImVec2& size)
{
    auto& buf = it.InputBuffer;
    if (buf.size() != (size_t)it.MaxLength + 1)
    {
        buf.resize(it.MaxLength + 1, '\0');
        strncpy(buf.data(), it.Text.c_str(), it.MaxLength);
    }

    ImGuiInputTextFlags flags = (it.ReadOnly ? ImGuiInputTextFlags_ReadOnly : 0) | (it.Password ? ImGuiInputTextFlags_Password : 0);
    bool changed = false;
    if (it.Multiline)
        changed = ImGui::InputTextMultiline(it.Label.c_str(), buf.data(), buf.size(), size, flags);
    else
    {
        ImGui::SetNextItemWidth(size.x);
        changed = ImGui::InputText(it.Label.c_str(), buf.data(), buf.size(), flags);
    }
    if (changed) { it.Text = buf.data(); wc.ValueChanged = true; }

    return ImGui::IsItemActive();
}

void RenderProgressBar(const ProgressBarData& pb, WidgetComponent& wc, const ImVec2& size)
{
    std::string overlay = pb.OverlayText;
    if (overlay.empty() && pb.ShowPercentage)
        overlay = std::to_string((int)(pb.Progress * 100)) + "%";

    ImGui::ProgressBar(pb.Progress, size, overlay.c_str());
}

bool RenderComboBox(ComboBoxData& cb, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    const char* preview = (cb.SelectedIndex >= 0 && cb.SelectedIndex < (int)cb.Items.size()) ? cb.Items[cb.SelectedIndex].c_str() : "";
    if (ImGui::BeginCombo(cb.Label.c_str(), preview))
    {
        for (int i = 0; i < (int)cb.Items.size(); i++)
            if (ImGui::Selectable(cb.Items[i].c_str(), i == cb.SelectedIndex)) { cb.SelectedIndex = i; wc.ValueChanged = true; }
        ImGui::EndCombo();
    }
    return ImGui::IsItemActive();
}

bool RenderImageButton(ImageButtonData& ib, WidgetComponent& wc, const ImVec2& size)
{
    TextureHandle textureHandle = ib.TextureHandle;
    if (textureHandle == 0 && !ib.TexturePath.empty())
    {
        textureHandle = ServiceLocator::Get<TextureSystem>().LoadTexture(ib.TexturePath);
        ib.TextureHandle = textureHandle;
    }

    if (textureHandle == 0) return false;

    auto tex = ServiceLocator::Get<TextureSystem>().GetTexture(textureHandle);
    if (!tex) return false;

    ImTextureID tid = (ImTextureID)(uintptr_t)tex->GetRendererID();
    if (ImGui::ImageButton(ib.Label.c_str(), tid, size, {0,1}, {1,0}, ToImVec4(ib.BackgroundColor), ToImVec4(ib.TintColor)))
        wc.PressedThisFrame = true;
    return ImGui::IsItemActive();
}

bool RenderRadioButton(RadioButtonData& rb, WidgetComponent& wc)
{
    for (int i = 0; i < (int)rb.Options.size(); i++)
    {
        if (ImGui::RadioButton(rb.Options[i].c_str(), rb.SelectedIndex == i)) { rb.SelectedIndex = i; wc.ValueChanged = true; }
        if (rb.Horizontal && i < (int)rb.Options.size() - 1) ImGui::SameLine();
    }
    return ImGui::IsItemActive();
}

bool RenderColorPicker(ColorPickerData& cp, WidgetComponent& wc)
{
    float col[4] = {cp.SelectedColor.r / 255.f, cp.SelectedColor.g / 255.f, cp.SelectedColor.b / 255.f, cp.SelectedColor.a / 255.f};
    if (cp.ShowPicker ? ImGui::ColorPicker4(cp.Label.c_str(), col) : ImGui::ColorEdit4(cp.Label.c_str(), col))
    {
        cp.SelectedColor = {(uint8_t)(col[0]*255), (uint8_t)(col[1]*255), (uint8_t)(col[2]*255), (uint8_t)(col[3]*255)};
        wc.ValueChanged = true;
    }
    return ImGui::IsItemActive();
}

void RenderSeparator(const SeparatorData& sep)
{
    ImGui::PushStyleColor(ImGuiCol_Separator, ToImVec4(sep.LineColor));
    ImGui::Separator();
    ImGui::PopStyleColor();
}

bool RenderDragFloat(DragFloatData& df, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    wc.ValueChanged = ImGui::DragFloat(df.Label.c_str(), &df.Value, df.Speed, df.Min, df.Max, df.Format.c_str());
    return ImGui::IsItemActive();
}

bool RenderDragInt(DragIntData& di, WidgetComponent& wc, const ImVec2& size)
{
    ImGui::SetNextItemWidth(size.x);
    wc.ValueChanged = ImGui::DragInt(di.Label.c_str(), &di.Value, di.Speed, di.Min, di.Max, di.Format.c_str());
    return ImGui::IsItemActive();
}

bool RenderTreeNode(TreeNodeData& tn, WidgetComponent& wc)
{
    ImGuiTreeNodeFlags flags = (tn.DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0) |
                               (tn.IsLeaf ? ImGuiTreeNodeFlags_Leaf : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;
    tn.IsOpen = ImGui::TreeNodeEx(tn.Label.c_str(), flags);
    return ImGui::IsItemActive();
}

bool RenderCollapsingHeader(CollapsingHeaderData& ch, WidgetComponent& wc)
{
    ch.IsOpen = ImGui::CollapsingHeader(ch.Label.c_str(), ch.DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    return ImGui::IsItemActive();
}

bool RenderPlotLines(const PlotLinesData& pl, WidgetComponent& wc)
{
    ImGui::PlotLines(pl.Label.c_str(), pl.Values.data(), (int)pl.Values.size(), 0, pl.OverlayText.c_str(), pl.ScaleMin, pl.ScaleMax, {pl.GraphSize.x, pl.GraphSize.y});
    return ImGui::IsItemActive();
}

bool RenderPlotHistogram(const PlotHistogramData& ph, WidgetComponent& wc)
{
    ImGui::PlotHistogram(ph.Label.c_str(), ph.Values.data(), (int)ph.Values.size(), 0, ph.OverlayText.c_str(), ph.ScaleMin, ph.ScaleMax, {ph.GraphSize.x, ph.GraphSize.y});
    return ImGui::IsItemActive();
}

void RenderTabBar(Entity tabBarEntity, const TabBarData& tb, WidgetComponent& wc, entt::registry& registry)
{
    ImGuiTabBarFlags flags = (tb.Reorderable ? ImGuiTabBarFlags_Reorderable : 0) |
                             (tb.AutoSelectNewTabs ? ImGuiTabBarFlags_AutoSelectNewTabs : 0);
    if (!ImGui::BeginTabBar(tb.Label.c_str(), flags)) return;

    std::vector<entt::entity> tabItems;
    if (tabBarEntity.HasComponent<HierarchyComponent>())
    {
        auto& hierarchy = tabBarEntity.GetComponent<HierarchyComponent>();
        for (auto childID : hierarchy.Children)
        {
            if (registry.valid(childID) && registry.all_of<WidgetComponent, ControlComponent>(childID))
            {
                auto& childWidget = registry.get<WidgetComponent>(childID);
                if (childWidget.Data.index() == 18) // TabItemData index
                {
                    tabItems.push_back(childID);
                }
            }
        }
        std::sort(tabItems.begin(), tabItems.end(), [&](entt::entity a, entt::entity b) {
            return registry.get<ControlComponent>(a).ZOrder < registry.get<ControlComponent>(b).ZOrder;
        });
    }

    for (auto childID : tabItems)
    {
        auto& childWidget = registry.get<WidgetComponent>(childID);
        auto& ti = std::get<TabItemData>(childWidget.Data);
        std::string label = ti.Label + "##" + std::to_string((uint32_t)childID);
        if (ImGui::BeginTabItem(label.c_str(), &ti.IsOpen)) { ti.Selected = true; ImGui::EndTabItem(); }
        else ti.Selected = false;
    }

    ImGui::EndTabBar();
}

} // namespace CHEngine::UI
