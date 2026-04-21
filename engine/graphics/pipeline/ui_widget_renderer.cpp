#include "ui_widget_renderer.h"
#include "ui_renderer.h"
#include "engine/graphics/texture_system.h"
#include "engine/core/log.h"
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
        ImFont* font = UIRenderer::Get().GetFontRegistry().GetFont(fontName, text.FontSize);
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

void RenderPanel(const PanelControl& panel, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImU32 bgColor    = ImGui::GetColorU32(ToImVec4(panel.Style.BackgroundColor));
    ImU32 borderCol  = ImGui::GetColorU32(ToImVec4(panel.Style.BorderColor));
    ImVec2 pMax      = {pos.x + size.x, pos.y + size.y};

    TextureHandle textureHandle = panel.TextureHandle;
    if (textureHandle == 0 && !panel.TexturePath.empty())
    {
        textureHandle = TextureSystem::Get().LoadTexture(panel.TexturePath);
    }

    if (textureHandle != 0)
    {
        auto texture = TextureSystem::Get().GetTexture(textureHandle);
        if (texture && texture->IsReady())
        {
            ImTextureID texId = (ImTextureID)(uintptr_t)texture->GetRendererID();
            // UI textures are loaded with stb vertical flip enabled; invert V in ImGui sampling to keep them upright.
            dl->AddImageRounded(texId, pos, pMax, {0,1}, {1,0}, IM_COL32_WHITE, panel.Style.Rounding);
        }
    }
    else if (panel.Style.UseGradient)
    {
        ImU32 gradColor = ImGui::GetColorU32(ToImVec4(panel.Style.GradientColor));
        dl->AddRectFilledMultiColor(pos, pMax, bgColor, bgColor, gradColor, gradColor);
    }
    else
    {
        dl->AddRectFilled(pos, pMax, bgColor, panel.Style.Rounding);
    }

    if (panel.Style.BorderSize > 0.0f)
        dl->AddRect(pos, pMax, borderCol, panel.Style.Rounding, 0, panel.Style.BorderSize);
}

void RenderLabel(const LabelControl& label, const ImVec2& size)
{
    StyleCounts c;
    PushTextStyle(label.Style, c);

    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + size.x);
    ImVec2 ts = ImGui::CalcTextSize(label.Text.c_str(), nullptr, true, size.x);

    float sx = 0, sy = 0;
    if (label.Style.Horizontal == HorizontalAlignment::Center)  sx = (size.x - ts.x) * 0.5f;
    else if (label.Style.Horizontal == HorizontalAlignment::Right) sx = size.x - ts.x;
    if (label.Style.Vertical == VerticalAlignment::Center)    sy = (size.y - ts.y) * 0.5f;
    else if (label.Style.Vertical == VerticalAlignment::Bottom)  sy = size.y - ts.y;

    ImGui::SetCursorPos({ImGui::GetCursorPosX() + sx, ImGui::GetCursorPosY() + sy});
    ImGui::TextUnformatted(label.Text.c_str());
    ImGui::PopTextWrapPos();

    PopUIStyle(c);
}

bool RenderButton(Entity entity, ButtonControl& button, const ImVec2& size)
{
    ImGui::PushID((int)entity);
    ImGui::InvisibleButton(button.Label.c_str(), size);

    button.IsHovered = ImGui::IsItemHovered();
    button.IsDown    = ImGui::IsItemActive();
    if (ImGui::IsItemClicked())
    {
        CH_CORE_INFO("UI: Button '{}' clicked", button.Label);
        button.PressedThisFrame = true;
    }

    bool handled = ImGui::IsItemActive();

    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImVec2 pos       = ImGui::GetItemRectMin();
    ImVec2 aSize     = size;

    if (button.Style.State.CurrentScale != 1.0f)
    {
        ImVec2 center = {pos.x + size.x * 0.5f, pos.y + size.y * 0.5f};
        aSize.x *= button.Style.State.CurrentScale;
        aSize.y *= button.Style.State.CurrentScale;
        pos.x = center.x - aSize.x * 0.5f;
        pos.y = center.y - aSize.y * 0.5f;
    }

    ImVec2 pMax = {pos.x + aSize.x, pos.y + aSize.y};
    ImU32 color = ImGui::GetColorU32(ToImVec4(button.Style.State.CurrentColor));
    dl->AddRectFilled(pos, pMax, color, button.Style.Rounding);

    if (button.Style.BorderSize > 0.0f)
        dl->AddRect(pos, pMax, ImGui::GetColorU32(ToImVec4(button.Style.BorderColor)), button.Style.Rounding, 0, button.Style.BorderSize);

    StyleCounts c;
    PushTextStyle(button.Text, c);
    ImVec2 ts  = ImGui::CalcTextSize(button.Label.c_str());
    ImVec2 tp  = {pos.x + (aSize.x - ts.x) * 0.5f, pos.y + (aSize.y - ts.y) * 0.5f};
    dl->AddText(tp, ImGui::GetColorU32(ImGuiCol_Text), button.Label.c_str());
    PopUIStyle(c);

    ImGui::PopID();
    return handled;
}

bool RenderSlider(SliderControl& slider, const ImVec2& size)
{
    StyleCounts c = PushUIStyle(slider.Style);
    PushTextStyle(slider.Text, c);
    ImGui::SetNextItemWidth(size.x);
    slider.Changed = ImGui::SliderFloat(slider.Label.c_str(), &slider.Value, slider.Min, slider.Max);
    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

bool RenderCheckbox(CheckboxControl& cb)
{
    StyleCounts c = PushUIStyle(cb.Style);
    PushTextStyle(cb.Text, c);
    cb.Changed = ImGui::Checkbox(cb.Label.c_str(), &cb.Checked);
    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

void RenderImage(ImageControl& image, const ImVec2& size)
{
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImVec2 pos      = ImGui::GetCursorScreenPos();
    ImVec2 aSize    = size;

    ImGui::InvisibleButton("##img", size);
    image.IsHovered = ImGui::IsItemHovered();
    image.IsDown    = ImGui::IsItemActive();

    if (image.Style.State.CurrentScale != 1.0f)
    {
        ImVec2 center = {pos.x + size.x * 0.5f, pos.y + size.y * 0.5f};
        aSize.x *= image.Style.State.CurrentScale;
        aSize.y *= image.Style.State.CurrentScale;
        pos.x = center.x - aSize.x * 0.5f;
        pos.y = center.y - aSize.y * 0.5f;
    }

    ImVec2 pMax = {pos.x + aSize.x, pos.y + aSize.y};

    TextureHandle textureHandle = image.TextureHandle;
    if (textureHandle == 0 && !image.TexturePath.empty())
    {
        textureHandle = TextureSystem::Get().LoadTexture(image.TexturePath);
        image.TextureHandle = textureHandle;
    }

    if (textureHandle != 0)
    {
        auto tex = TextureSystem::Get().GetTexture(textureHandle);
        if (tex && tex->IsReady())
        {
            ImTextureID tid = (ImTextureID)(uintptr_t)tex->GetRendererID();
            dl->AddImageRounded(tid, pos, pMax, {0,1}, {1,0}, ImGui::GetColorU32(ToImVec4(image.TintColor)), image.Style.Rounding);
        }
    }
    else
    {
        ImU32 bg = ImGui::GetColorU32(ToImVec4(image.Style.State.CurrentColor));
        if (image.Style.UseGradient)
        {
            ImU32 gc = ImGui::GetColorU32(ToImVec4(image.Style.GradientColor));
            dl->AddRectFilledMultiColor(pos, pMax, bg, bg, gc, gc);
        }
        else
        {
            dl->AddRectFilled(pos, pMax, bg, image.Style.Rounding);
        }
    }

    if (image.Style.BorderSize > 0.0f)
        dl->AddRect(pos, pMax, ImGui::GetColorU32(ToImVec4(image.BorderColor)), image.Style.Rounding, 0, image.Style.BorderSize);
}

bool RenderInputText(Entity entity, InputTextControl& it, const ImVec2& size)
{
    StyleCounts c = PushUIStyle(it.BoxStyle, !it.ReadOnly);
    PushTextStyle(it.Style, c);

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
    if (changed) { it.Text = buf.data(); it.Changed = true; }

    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

void RenderProgressBar(const ProgressBarControl& pb, const ImVec2& size)
{
    StyleCounts c = PushUIStyle(pb.BarStyle);
    PushTextStyle(pb.Style, c);

    std::string overlay = pb.OverlayText;
    if (overlay.empty() && pb.ShowPercentage)
        overlay = std::to_string((int)(pb.Progress * 100)) + "%";

    ImGui::ProgressBar(pb.Progress, size, overlay.c_str());
    PopUIStyle(c);
}

bool RenderComboBox(ComboBoxControl& cb, const ImVec2& size)
{
    StyleCounts c = PushUIStyle(cb.BoxStyle);
    PushTextStyle(cb.Style, c);
    ImGui::SetNextItemWidth(size.x);
    const char* preview = (cb.SelectedIndex >= 0 && cb.SelectedIndex < (int)cb.Items.size()) ? cb.Items[cb.SelectedIndex].c_str() : "";
    if (ImGui::BeginCombo(cb.Label.c_str(), preview))
    {
        for (int i = 0; i < (int)cb.Items.size(); i++)
            if (ImGui::Selectable(cb.Items[i].c_str(), i == cb.SelectedIndex)) { cb.SelectedIndex = i; cb.Changed = true; }
        ImGui::EndCombo();
    }
    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

bool RenderImageButton(ImageButtonControl& ib, const ImVec2& size)
{
    TextureHandle textureHandle = ib.TextureHandle;
    if (textureHandle == 0 && !ib.TexturePath.empty())
    {
        textureHandle = TextureSystem::Get().LoadTexture(ib.TexturePath);
        ib.TextureHandle = textureHandle;
    }

    if (textureHandle == 0) return false;

    auto tex = TextureSystem::Get().GetTexture(textureHandle);
    if (!tex) return false;

    ImTextureID tid = (ImTextureID)(uintptr_t)tex->GetRendererID();
    if (ImGui::ImageButton(ib.Label.c_str(), tid, size, {0,1}, {1,0}, ToImVec4(ib.BackgroundColor), ToImVec4(ib.TintColor)))
        ib.PressedThisFrame = true;
    return ImGui::IsItemActive();
}

bool RenderRadioButton(RadioButtonControl& rb)
{
    StyleCounts c;
    PushTextStyle(rb.Style, c);
    for (int i = 0; i < (int)rb.Options.size(); i++)
    {
        if (ImGui::RadioButton(rb.Options[i].c_str(), rb.SelectedIndex == i)) { rb.SelectedIndex = i; rb.Changed = true; }
        if (rb.Horizontal && i < (int)rb.Options.size() - 1) ImGui::SameLine();
    }
    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

bool RenderColorPicker(ColorPickerControl& cp)
{
    float col[4] = {cp.SelectedColor.r / 255.f, cp.SelectedColor.g / 255.f, cp.SelectedColor.b / 255.f, cp.SelectedColor.a / 255.f};
    if (cp.ShowPicker ? ImGui::ColorPicker4(cp.Label.c_str(), col) : ImGui::ColorEdit4(cp.Label.c_str(), col))
    {
        cp.SelectedColor = {(uint8_t)(col[0]*255), (uint8_t)(col[1]*255), (uint8_t)(col[2]*255), (uint8_t)(col[3]*255)};
        cp.Changed = true;
    }
    return ImGui::IsItemActive();
}

void RenderSeparator(const SeparatorControl& sep)
{
    ImGui::PushStyleColor(ImGuiCol_Separator, ToImVec4(sep.LineColor));
    ImGui::Separator();
    ImGui::PopStyleColor();
}

bool RenderDragFloat(DragFloatControl& df, const ImVec2& size)
{
    StyleCounts c = PushUIStyle(df.BoxStyle);
    PushTextStyle(df.Style, c);
    ImGui::SetNextItemWidth(size.x);
    df.Changed = ImGui::DragFloat(df.Label.c_str(), &df.Value, df.Speed, df.Min, df.Max, df.Format.c_str());
    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

bool RenderDragInt(DragIntControl& di, const ImVec2& size)
{
    StyleCounts c = PushUIStyle(di.BoxStyle);
    PushTextStyle(di.Style, c);
    ImGui::SetNextItemWidth(size.x);
    di.Changed = ImGui::DragInt(di.Label.c_str(), &di.Value, di.Speed, di.Min, di.Max, di.Format.c_str());
    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

bool RenderTreeNode(TreeNodeControl& tn)
{
    StyleCounts c;
    PushTextStyle(tn.Style, c);
    ImGuiTreeNodeFlags flags = (tn.DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0) |
                               (tn.IsLeaf ? ImGuiTreeNodeFlags_Leaf : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;
    tn.IsOpen = ImGui::TreeNodeEx(tn.Label.c_str(), flags);
    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

bool RenderCollapsingHeader(CollapsingHeaderControl& ch)
{
    StyleCounts c;
    PushTextStyle(ch.Style, c);
    ch.IsOpen = ImGui::CollapsingHeader(ch.Label.c_str(), ch.DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

bool RenderPlotLines(const PlotLinesControl& pl)
{
    StyleCounts c = PushUIStyle(pl.BoxStyle);
    PushTextStyle(pl.Style, c);
    ImGui::PlotLines(pl.Label.c_str(), pl.Values.data(), (int)pl.Values.size(), 0, pl.OverlayText.c_str(), pl.ScaleMin, pl.ScaleMax, {pl.GraphSize.x, pl.GraphSize.y});
    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

bool RenderPlotHistogram(const PlotHistogramControl& ph)
{
    StyleCounts c = PushUIStyle(ph.BoxStyle);
    PushTextStyle(ph.Style, c);
    ImGui::PlotHistogram(ph.Label.c_str(), ph.Values.data(), (int)ph.Values.size(), 0, ph.OverlayText.c_str(), ph.ScaleMin, ph.ScaleMax, {ph.GraphSize.x, ph.GraphSize.y});
    bool handled = ImGui::IsItemActive();
    PopUIStyle(c);
    return handled;
}

void RenderTabBar(Entity tabBarEntity, const TabBarControl& tb, entt::registry& registry)
{
    ImGuiTabBarFlags flags = (tb.Reorderable ? ImGuiTabBarFlags_Reorderable : 0) |
                             (tb.AutoSelectNewTabs ? ImGuiTabBarFlags_AutoSelectNewTabs : 0);
    if (!ImGui::BeginTabBar(tb.Label.c_str(), flags)) return;

    std::vector<entt::entity> tabItems;
    if (tabBarEntity.HasComponent<HierarchyComponent>())
    {
        auto& hierarchy = tabBarEntity.GetComponent<HierarchyComponent>();
        for (auto childID : hierarchy.Children)
            if (registry.valid(childID) && registry.all_of<TabItemControl, ControlComponent>(childID))
                tabItems.push_back(childID);
        std::sort(tabItems.begin(), tabItems.end(), [&](entt::entity a, entt::entity b) {
            return registry.get<ControlComponent>(a).ZOrder < registry.get<ControlComponent>(b).ZOrder;
        });
    }

    for (auto childID : tabItems)
    {
        auto& ti = registry.get<TabItemControl>(childID);
        std::string label = ti.Label + "##" + std::to_string((uint32_t)childID);
        if (ImGui::BeginTabItem(label.c_str(), &ti.IsOpen)) { ti.Selected = true; ImGui::EndTabItem(); }
        else ti.Selected = false;
    }

    ImGui::EndTabBar();
}

} // namespace CHEngine::UI
