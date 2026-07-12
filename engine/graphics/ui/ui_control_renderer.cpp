#include "ui_control_renderer.h"
#include "ui_renderer.h"
#include "ui_font_registry.h"

#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/scene/components/tag_component.h"
#include "imgui_internal.h" // For low-level access to clipping and input if needed

namespace Chained
{

// ---------------------------------------------------------------------------
// Color conversion helpers
// ---------------------------------------------------------------------------
inline ImVec4 ToImVec4(const Color& c)
{
    return ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}

inline ImU32 ToImU32(const Color& c)
{
    return IM_COL32(c.r, c.g, c.b, c.a);
}

// Returns the correct background color based on the active control state
static ImU32 GetControlColor(const UIStyle& style, const UIControlComponent& wc)
{
    if (wc.IsDown)    return ToImU32(style.PressedColor);
    if (wc.IsHovered) return ToImU32(style.HoverColor);
    return ToImU32(style.BackgroundColor);
}

// ---------------------------------------------------------------------------
// Universal aligned text renderer
// ---------------------------------------------------------------------------
static void RenderAlignedTextureText(ImDrawList* dl, ImFont* font, float fontSize, const std::string& text, 
                                     const ImVec2& pos, const ImVec2& size, const TextStyle& textStyle)
{
    if (text.empty()) return;

    // Measure text size using the selected font
    ImVec2 textSize;
    if (font)
    {
        ImGui::PushFont(font);
        textSize = ImGui::CalcTextSize(text.c_str());
        ImGui::PopFont();
    }
    else
    {
        textSize = ImGui::CalcTextSize(text.c_str());
    }

    // Compute local offset relative to the widget bounding box
    ImVec2 textPos = pos;

    // Horizontal alignment
    if (textStyle.Horizontal == HorizontalAlignment::Center)
        textPos.x += (size.x - textSize.x) * 0.5f;
    else if (textStyle.Horizontal == HorizontalAlignment::Right)
        textPos.x += (size.x - textSize.x);

    // Vertical alignment
    if (textStyle.Vertical == VerticalAlignment::Center)
        textPos.y += (size.y - textSize.y) * 0.5f;
    else if (textStyle.Vertical == VerticalAlignment::Bottom)
        textPos.y += (size.y - textSize.y);

    // Draw text shadow (if enabled)
    if (textStyle.Shadow)
    {
        ImVec2 shadowPos = { textPos.x + textStyle.ShadowOffset, textPos.y + textStyle.ShadowOffset };
        if (font) dl->AddText(font, fontSize, shadowPos, ToImU32(textStyle.ShadowColor), text.c_str());
        else      dl->AddText(shadowPos, ToImU32(textStyle.ShadowColor), text.c_str());
    }

    // Draw main text
    if (font) dl->AddText(font, fontSize, textPos, ToImU32(textStyle.TextColor), text.c_str());
    else      dl->AddText(textPos, ToImU32(textStyle.TextColor), text.c_str());
}

// ---------------------------------------------------------------------------
// Low-level control rendering methods
// ---------------------------------------------------------------------------

static std::shared_ptr<TextureAsset> ResolveTexture(AssetHandle& handle, const std::string& path)
{
    auto* am = ServiceLocator::Get<AssetManager>();
    if (!am) return nullptr;

    if (handle == 0 && !path.empty())
    {
        auto asset = am->Get<TextureAsset>(path);
        if (asset)
            handle = static_cast<AssetHandle>(asset->GetID());
        return asset;
    }

    if (handle != 0)
        return am->Get<TextureAsset>(handle);

    return nullptr;
}

static bool RenderPanel(PanelData& panel, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};

    auto textureAsset = ResolveTexture(panel.TextureHandle, panel.TexturePath);
    if (textureAsset && textureAsset->GetState() == AssetState::Ready)
    {
        ImTextureID texId = (ImTextureID)(uintptr_t)textureAsset->GetTexture()->GetNativeHandle();
        dl->AddImageRounded(texId, pos, pMax, {0,0}, {1,1}, IM_COL32_WHITE, wc.BoxStyle.Rounding);
    }
    else
    {
        dl->AddRectFilled(pos, pMax, ToImU32(wc.BoxStyle.BackgroundColor), wc.BoxStyle.Rounding);
    }

    if (wc.BoxStyle.BorderSize > 0.0f)
    {
        dl->AddRect(pos, pMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);
    }
    return false;
}

static bool RenderButton(const ButtonData& button, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};

    // Render button background based on InputSystem state
    ImU32 btnColor = GetControlColor(wc.BoxStyle, wc);
    dl->AddRectFilled(pos, pMax, btnColor, wc.BoxStyle.Rounding);

    if (wc.BoxStyle.BorderSize > 0.0f)
    {
        dl->AddRect(pos, pMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);
    }

    // Draw text inside the button
    RenderAlignedTextureText(dl, font, textStyle.FontSize, button.Label, pos, size, textStyle);

    return wc.PressedThisFrame;
}

static bool RenderLabel(const LabelData& label, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    RenderAlignedTextureText(dl, font, textStyle.FontSize, label.Text, pos, size, textStyle);
    return false;
}

static bool RenderCheckbox(CheckboxData& cb, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    // Compute checkbox square (sized to match UI element height)
    float boxSize = size.y > 0.0f ? size.y : 18.0f;
    ImVec2 boxMax = { pos.x + boxSize, pos.y + boxSize };
    
    ImU32 boxColor = GetControlColor(wc.BoxStyle, wc);
    dl->AddRectFilled(pos, boxMax, boxColor, wc.BoxStyle.Rounding);
    dl->AddRect(pos, boxMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, 1.0f);

    // If checked — draw inner marker
    if (cb.Checked)
    {
        float pad = boxSize * 0.25f;
        dl->AddRectFilled({pos.x + pad, pos.y + pad}, {boxMax.x - pad, boxMax.y - pad}, ToImU32(textStyle.TextColor), wc.BoxStyle.Rounding);
    }

    // Offset checkbox text to the right of the square
    ImVec2 textPos = { pos.x + boxSize + 8.0f, pos.y };
    ImVec2 textSize = { size.x - boxSize - 8.0f, size.y };
    RenderAlignedTextureText(dl, font, textStyle.FontSize, cb.Label, textPos, textSize, textStyle);

    // If user clicked the widget (handled by InputSystem)
    if (wc.PressedThisFrame)
    {
        cb.Checked = !cb.Checked;
        return true;
    }

    return false;
}

static bool RenderSlider(SliderData& slider, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    // Keep default ImGui input but hard-cap its width (hide the "##" label)
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(size.x);
    
    // Push style vars to ImGui stack temporarily
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, wc.BoxStyle.Rounding);
    bool changed = ImGui::SliderFloat("##slider", &slider.Value, slider.Min, slider.Max, "%.2f");
    ImGui::PopStyleVar();
    
    return changed;
}

static bool RenderProgressBar(const ProgressBarData& pb, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};

    // Bar background
    dl->AddRectFilled(pos, pMax, ToImU32(wc.BoxStyle.BackgroundColor), wc.BoxStyle.Rounding);

    // Progress fill
    float currentProgressWidth = size.x * std::clamp(pb.Progress, 0.0f, 1.0f);
    if (currentProgressWidth > 0.0f)
    {
        ImVec2 progressMax = { pos.x + currentProgressWidth, pos.y + size.y };
        dl->AddRectFilled(pos, progressMax, ToImU32(wc.BoxStyle.HoverColor), wc.BoxStyle.Rounding);
    }

    if (wc.BoxStyle.BorderSize > 0.0f)
    {
        dl->AddRect(pos, pMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);
    }

    return false;
}

static bool RenderImage(ImageData& img, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};

    auto textureAsset = ResolveTexture(img.TextureHandle, img.TexturePath);
    if (textureAsset && textureAsset->GetState() == AssetState::Ready)
    {
        ImTextureID texId = (ImTextureID)(uintptr_t)textureAsset->GetTexture()->GetNativeHandle();
        dl->AddImageRounded(texId, pos, pMax, {0,0}, {1,1}, ToImU32(img.TintColor), wc.BoxStyle.Rounding);
    }
    else if (!img.TexturePath.empty())
    {
        // Asset is still loading — draw a placeholder tinted with the tint color
        dl->AddRectFilled(pos, pMax, IM_COL32(60, 60, 60, 200), wc.BoxStyle.Rounding);
    }
    else
    {
        dl->AddRectFilled(pos, pMax, ToImU32(wc.BoxStyle.BackgroundColor), wc.BoxStyle.Rounding);
    }

    if (wc.BoxStyle.BorderSize > 0.0f)
        dl->AddRect(pos, pMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);

    return false;
}

static bool RenderImageButton(ImageButtonData& imgBtn, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};

    ImU32 btnColor = GetControlColor(wc.BoxStyle, wc);
    dl->AddRectFilled(pos, pMax, btnColor, wc.BoxStyle.Rounding);

    float pad = imgBtn.FramePadding >= 0 ? static_cast<float>(imgBtn.FramePadding) : 4.0f;
    ImVec2 imgPos = { pos.x + pad, pos.y + pad };
    ImVec2 imgMax = { pMax.x - pad, pMax.y - pad };

    auto textureAsset = ResolveTexture(imgBtn.TextureHandle, imgBtn.TexturePath);
    if (textureAsset && textureAsset->GetState() == AssetState::Ready)
    {
        ImTextureID texId = (ImTextureID)(uintptr_t)textureAsset->GetTexture()->GetNativeHandle();
        dl->AddImageRounded(texId, imgPos, imgMax, {0,0}, {1,1}, ToImU32(imgBtn.TintColor), wc.BoxStyle.Rounding);
    }

    if (!imgBtn.Label.empty())
        RenderAlignedTextureText(dl, font, textStyle.FontSize, imgBtn.Label, pos, size, textStyle);

    if (wc.BoxStyle.BorderSize > 0.0f)
        dl->AddRect(pos, pMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);

    return wc.PressedThisFrame;
}

static bool RenderComboBox(ComboBoxData& combo, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(size.x);

    const char* preview = (combo.SelectedIndex >= 0 && combo.SelectedIndex < (int)combo.Items.size())
        ? combo.Items[combo.SelectedIndex].c_str() : "Select...";

    bool changed = false;
    if (ImGui::BeginCombo("##combo", preview))
    {
        for (int i = 0; i < (int)combo.Items.size(); i++)
        {
            bool isSelected = (combo.SelectedIndex == i);
            if (ImGui::Selectable(combo.Items[i].c_str(), isSelected))
            {
                combo.SelectedIndex = i;
                changed = true;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

static bool RenderInputText(InputTextData& input, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    if (input.InputBuffer.empty())
    {
        input.InputBuffer.resize(static_cast<size_t>(input.MaxLength) + 1, '\0');
        std::copy(input.Text.begin(),
                  input.Text.begin() + std::min(input.Text.size(), static_cast<size_t>(input.MaxLength)),
                  input.InputBuffer.begin());
    }

    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(size.x);

    ImGuiInputTextFlags flags = 0;
    if (input.ReadOnly)   flags |= ImGuiInputTextFlags_ReadOnly;
    if (input.Password)   flags |= ImGuiInputTextFlags_Password;

    bool changed = false;
    if (input.Multiline)
    {
        changed = ImGui::InputTextMultiline("##inputtext", input.InputBuffer.data(),
                                            input.InputBuffer.size(), size, flags);
    }
    else
    {
        changed = ImGui::InputText("##inputtext", input.InputBuffer.data(),
                                   input.InputBuffer.size(), flags);
    }

    if (changed)
        input.Text = input.InputBuffer.data();

    return changed;
}

static bool RenderSeparator(const SeparatorData& sep, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float midY = pos.y + size.y * 0.5f;
    dl->AddLine({pos.x, midY}, {pos.x + size.x, midY}, ToImU32(sep.LineColor), sep.Thickness);
    return false;
}

static bool RenderRadioButton(RadioButtonData& radio, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    bool changed = false;

    if (radio.Horizontal)
    {
        float itemW = (radio.Options.empty()) ? size.x : size.x / static_cast<float>(radio.Options.size());
        for (int i = 0; i < (int)radio.Options.size(); i++)
        {
            ImVec2 itemPos = {pos.x + itemW * i, pos.y};
            float radius = std::min(size.y * 0.35f, 8.0f);
            ImVec2 circleCenter = {itemPos.x + radius + 4.0f, pos.y + size.y * 0.5f};
            ImU32 circleColor = (i == radio.SelectedIndex) ? ToImU32(textStyle.TextColor) : ToImU32(wc.BoxStyle.BorderColor);
            dl->AddCircle(circleCenter, radius, circleColor, 12, 1.5f);
            if (i == radio.SelectedIndex)
                dl->AddCircleFilled(circleCenter, radius * 0.5f, ToImU32(textStyle.TextColor));
            ImVec2 labelPos = {circleCenter.x + radius + 4.0f, pos.y};
            ImVec2 labelSize = {itemW - radius * 2.0f - 12.0f, size.y};
            RenderAlignedTextureText(dl, font, textStyle.FontSize, radio.Options[i], labelPos, labelSize, textStyle);

            ImVec2 itemMax = {itemPos.x + itemW, pos.y + size.y};
            ImGui::SetCursorScreenPos(itemPos);
            if (ImGui::InvisibleButton(("##radio" + std::to_string(i)).c_str(), {itemW, size.y}))
            {
                radio.SelectedIndex = i;
                changed = true;
            }
        }
    }
    else
    {
        float itemH = (radio.Options.empty()) ? size.y : size.y / static_cast<float>(radio.Options.size());
        for (int i = 0; i < (int)radio.Options.size(); i++)
        {
            ImVec2 itemPos = {pos.x, pos.y + itemH * i};
            float radius = std::min(itemH * 0.35f, 8.0f);
            ImVec2 circleCenter = {itemPos.x + radius + 4.0f, itemPos.y + itemH * 0.5f};
            ImU32 circleColor = (i == radio.SelectedIndex) ? ToImU32(textStyle.TextColor) : ToImU32(wc.BoxStyle.BorderColor);
            dl->AddCircle(circleCenter, radius, circleColor, 12, 1.5f);
            if (i == radio.SelectedIndex)
                dl->AddCircleFilled(circleCenter, radius * 0.5f, ToImU32(textStyle.TextColor));
            ImVec2 labelPos = {circleCenter.x + radius + 4.0f, itemPos.y};
            ImVec2 labelSize = {size.x - radius * 2.0f - 12.0f, itemH};
            RenderAlignedTextureText(dl, font, textStyle.FontSize, radio.Options[i], labelPos, labelSize, textStyle);

            ImGui::SetCursorScreenPos(itemPos);
            if (ImGui::InvisibleButton(("##radio" + std::to_string(i)).c_str(), {size.x, itemH}))
            {
                radio.SelectedIndex = i;
                changed = true;
            }
        }
    }
    return changed;
}

static bool RenderColorPicker(ColorPickerData& picker, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    float col[4] = {
        picker.SelectedColor.r / 255.0f,
        picker.SelectedColor.g / 255.0f,
        picker.SelectedColor.b / 255.0f,
        picker.SelectedColor.a / 255.0f,
    };

    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoLabel;
    if (!picker.ShowAlpha)  flags |= ImGuiColorEditFlags_NoAlpha;
    if (!picker.ShowPicker) flags |= ImGuiColorEditFlags_NoPicker;

    bool changed = false;
    if (picker.ShowPicker)
    {
        changed = ImGui::ColorPicker4("##colorpicker", col, flags);
    }
    else
    {
        changed = ImGui::ColorEdit4("##coloredit", col, flags);
    }

    if (changed)
    {
        picker.SelectedColor = {
            static_cast<uint8_t>(col[0] * 255.0f),
            static_cast<uint8_t>(col[1] * 255.0f),
            static_cast<uint8_t>(col[2] * 255.0f),
            static_cast<uint8_t>(col[3] * 255.0f),
        };
    }
    return changed;
}

static bool RenderDragFloat(DragFloatData& drag, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(size.x);
    return ImGui::DragFloat("##dragfloat", &drag.Value, drag.Speed, drag.Min, drag.Max, drag.Format.c_str());
}

static bool RenderDragInt(DragIntData& drag, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(size.x);
    return ImGui::DragInt("##dragint", &drag.Value, drag.Speed, drag.Min, drag.Max, drag.Format.c_str());
}

static bool RenderTreeNode(TreeNodeData& node, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle)
{
    ImGui::SetCursorScreenPos(pos);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    if (node.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (node.IsLeaf)      flags |= ImGuiTreeNodeFlags_Leaf;
    node.IsOpen = ImGui::TreeNodeEx(node.Label.c_str(), flags);
    if (node.IsOpen)
        ImGui::TreePop();
    return false;
}

static bool RenderTabBar(TabBarData& bar, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGuiTabBarFlags flags = 0;
    if (bar.Reorderable)       flags |= ImGuiTabBarFlags_Reorderable;
    if (bar.AutoSelectNewTabs) flags |= ImGuiTabBarFlags_AutoSelectNewTabs;
    ImGui::BeginTabBar(bar.Label.c_str(), flags);
    ImGui::EndTabBar();
    return false;
}

static bool RenderTabItem(TabItemData& item, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    bool open = item.IsOpen;
    item.Selected = ImGui::BeginTabItem(item.Label.c_str(), item.IsOpen ? &open : nullptr);
    if (item.Selected)
        ImGui::EndTabItem();
    item.IsOpen = open;
    return false;
}

static bool RenderCollapsingHeader(CollapsingHeaderData& header, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGuiTreeNodeFlags flags = 0;
    if (header.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    header.IsOpen = ImGui::CollapsingHeader(header.Label.c_str(), flags);
    return false;
}

static bool RenderPlotLines(const PlotLinesData& plot, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    const char* overlay = plot.OverlayText.empty() ? nullptr : plot.OverlayText.c_str();
    ImGui::PlotLines("##plotlines", plot.Values.data(), static_cast<int>(plot.Values.size()),
                     0, overlay, plot.ScaleMin, plot.ScaleMax,
                     {size.x, size.y});
    return false;
}

static bool RenderPlotHistogram(const PlotHistogramData& plot, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    const char* overlay = plot.OverlayText.empty() ? nullptr : plot.OverlayText.c_str();
    ImGui::PlotHistogram("##plothisto", plot.Values.data(), static_cast<int>(plot.Values.size()),
                         0, overlay, plot.ScaleMin, plot.ScaleMax,
                         {size.x, size.y});
    return false;
}

static bool RenderVerticalLayoutGroup(const VerticalLayoutGroupData& layout, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    // Pure layout container — no visual output; children are positioned by UILayoutSystem.
    return false;
}

// ---------------------------------------------------------------------------
// Main UI Control Dispatcher
// ---------------------------------------------------------------------------
bool RenderControl(const UIFontRegistry& fontRegistry,
                   Entity entity, UIControlComponent& control, const ImVec2& screenPos, const ImVec2& size)
{
    // 1. Early return for empty control data
    if (std::holds_alternative<std::monostate>(control.Data))
        return false;

    // 2. Resolve the active font from the registry
    ImFont* activeFont = nullptr;
    if (!control.TextStyle.FontName.empty() && control.TextStyle.FontName != "Default")
    {
        activeFont = fontRegistry.GetFont(control.TextStyle.FontName, control.TextStyle.FontSize);
    }

    bool changed = false;

    // 3. Dispatch variant without ImGui spacing
    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ButtonData>)
            changed = RenderButton(arg, control, screenPos, size, activeFont, control.TextStyle);
        else if constexpr (std::is_same_v<T, PanelData>)
            changed = RenderPanel(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, LabelData>)
            changed = RenderLabel(arg, control, screenPos, size, activeFont, control.TextStyle);
        else if constexpr (std::is_same_v<T, CheckboxData>)
            changed = RenderCheckbox(arg, control, screenPos, size, activeFont, control.TextStyle);
        else if constexpr (std::is_same_v<T, SliderData>)
            changed = RenderSlider(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, ProgressBarData>)
            changed = RenderProgressBar(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, ImageData>)
            changed = RenderImage(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, ImageButtonData>)
            changed = RenderImageButton(arg, control, screenPos, size, activeFont, control.TextStyle);
        else if constexpr (std::is_same_v<T, ComboBoxData>)
            changed = RenderComboBox(arg, control, screenPos, size, activeFont, control.TextStyle);
        else if constexpr (std::is_same_v<T, InputTextData>)
            changed = RenderInputText(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, SeparatorData>)
            changed = RenderSeparator(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, RadioButtonData>)
            changed = RenderRadioButton(arg, control, screenPos, size, activeFont, control.TextStyle);
        else if constexpr (std::is_same_v<T, ColorPickerData>)
            changed = RenderColorPicker(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, DragFloatData>)
            changed = RenderDragFloat(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, DragIntData>)
            changed = RenderDragInt(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, TreeNodeData>)
            changed = RenderTreeNode(arg, control, screenPos, size, activeFont, control.TextStyle);
        else if constexpr (std::is_same_v<T, TabBarData>)
            changed = RenderTabBar(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, TabItemData>)
            changed = RenderTabItem(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, CollapsingHeaderData>)
            changed = RenderCollapsingHeader(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, PlotLinesData>)
            changed = RenderPlotLines(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, PlotHistogramData>)
            changed = RenderPlotHistogram(arg, control, screenPos, size);
        else if constexpr (std::is_same_v<T, VerticalLayoutGroupData>)
            changed = RenderVerticalLayoutGroup(arg, control, screenPos, size);
    }, control.Data);

    control.ValueChanged = changed;
    return true;
}

} // namespace Chained