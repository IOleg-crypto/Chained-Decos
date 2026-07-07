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

static bool RenderPanel(PanelData& panel, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};
    AssetHandle textureHandle = (AssetHandle)panel.TextureHandle;
    
    if (textureHandle != 0)
    {
        auto textureAsset = ServiceLocator::Get<AssetManager>()->Get<TextureAsset>(textureHandle);
        if (textureAsset && textureAsset->GetState() == AssetState::Ready)
        {
            ImTextureID texId = (ImTextureID)(uintptr_t)textureAsset->GetTexture()->GetRendererID();
            dl->AddImageRounded(texId, pos, pMax, {0,0}, {1,1}, IM_COL32_WHITE, wc.BoxStyle.Rounding);
        }
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
        activeFont = fontRegistry.GetFont(control.TextStyle.FontName, (int)control.TextStyle.FontSize);
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
        else
        {
            // Fallback: draw a bordered rectangle with type name for unhandled UI types
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRect(screenPos, ImVec2(screenPos.x + size.x, screenPos.y + size.y),
                        IM_COL32(100, 100, 100, 255), 0.0f, 0, 1.0f);
            // Show type info if available
            if (entity.HasComponent<TagComponent>())
            {
                const char* label = entity.GetComponent<TagComponent>().Tag.c_str();
                dl->AddText(ImVec2(screenPos.x + 4, screenPos.y + 2), IM_COL32(180, 180, 180, 255), label);
            }
        }
    }, control.Data);

    control.ValueChanged = changed;
    return true;
}

} // namespace Chained